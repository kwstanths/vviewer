#include "VulkanMaterials.hpp"

#include <zip.h>

#include "core/AssetManager.hpp"
#include "core/StringUtils.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanMaterials::VulkanMaterials(VulkanContext &ctx)
    : m_vkctx(ctx)
    , m_materialsStorage(VULKAN_LIMITS_MAX_MATERIALS)
{
}

VkResult VulkanMaterials::initResources()
{
    VULKAN_CHECK_CRITICAL(m_materialsStorage.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment));

    if (m_materialsStorage.blockSizeAligned() != sizeof(MaterialData)) {
        std::string error =
            "VulkanMaterials::initResources(): The Material GPU block size is different than the size of MaterialData. GPU Block Size "
            "= " +
            std::to_string(m_materialsStorage.blockSizeAligned());
        debug_tools::ConsoleCritical(error);
        return VK_ERROR_UNKNOWN;
    }

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout());

    return VK_SUCCESS;
}

VkResult VulkanMaterials::initSwapchainResources(uint32_t nImages)
{
    m_swapchainImages = nImages;

    VULKAN_CHECK_CRITICAL(m_materialsStorage.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages, 10));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));

    updateDescriptorSets();

    {
        auto &materialsSkybox = AssetManager::getInstance().materialsSkyboxMap();
        for (auto itr = materialsSkybox.begin(); itr != materialsSkybox.end(); ++itr) {
            auto material = static_cast<VulkanMaterialSkybox *>(itr->second);
            VULKAN_CHECK_CRITICAL(material->createDescriptors(m_vkctx.device(), m_descriptorPool, m_swapchainImages));
            material->updateDescriptorSets(m_vkctx.device(), m_swapchainImages);
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanMaterials::releaseResources()
{
    /* Destroy material data */
    {
        AssetManager::getInstance().materialsMap().reset();
    }

    m_materialsStorage.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanMaterials::releaseSwapchainResources()
{
    m_materialsStorage.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

void VulkanMaterials::updateBuffers(uint32_t index) const
{
    m_materialsStorage.updateBuffer(m_vkctx.device(), index);
}

VkBuffer VulkanMaterials::getBuffer(uint32_t index)
{
    return m_materialsStorage.buffer(index);
}

Material *VulkanMaterials::createMaterial(const AssetInfo &info, MaterialType type)
{
    auto &materials = AssetManager::getInstance().materialsMap();
    if (materials.isPresent(info.name)) {
        return materials.get(info.name);
    }

    Material *temp;
    switch (type) {
        case MaterialType::MATERIAL_PBR_STANDARD: {
            temp = new VulkanMaterialPBRStandard(info, m_materialsStorage);
            materials.add(temp);
            break;
        }
        case MaterialType::MATERIAL_LAMBERT: {
            temp = new VulkanMaterialLambert(info, m_materialsStorage);
            materials.add(temp);
            break;
        }
        case MaterialType::MATERIAL_VOLUME: {
            temp = new VulkanMaterialVolume(info, m_materialsStorage);
            materials.add(temp);
            break;
        }
        default: {
            debug_tools::ConsoleWarning("VulkanMaterialSystem::createMaterial(): Unexpected material");
            return nullptr;
        }
    }

    return temp;
}

Material *VulkanMaterials::createMaterialFromDisk(const AssetInfo &info, std::string stackDirectory, Textures &textures)
{
    auto material = createMaterial(info, MaterialType::MATERIAL_PBR_STANDARD);
    if (material == nullptr)
        return nullptr;

    auto albedo = textures.createTexture(AssetInfo(stackDirectory + "/albedo.png", info.source), ColorSpace::sRGB);
    auto ao = textures.createTexture(AssetInfo(stackDirectory + "/ao.png", info.source), ColorSpace::LINEAR);
    auto metallic = textures.createTexture(AssetInfo(stackDirectory + "/metallic.png", info.source), ColorSpace::LINEAR);
    auto normal = textures.createTexture(AssetInfo(stackDirectory + "/normal.png", info.source), ColorSpace::LINEAR);
    auto roughness = textures.createTexture(AssetInfo(stackDirectory + "/roughness.png", info.source), ColorSpace::LINEAR);

    if (albedo != nullptr)
        static_cast<MaterialPBRStandard *>(material)->setAlbedoTexture(albedo);
    if (ao != nullptr)
        static_cast<MaterialPBRStandard *>(material)->setAOTexture(ao);
    if (metallic != nullptr)
        static_cast<MaterialPBRStandard *>(material)->setMetallicTexture(metallic);
    if (normal != nullptr)
        static_cast<MaterialPBRStandard *>(material)->setNormalTexture(normal);
    if (roughness != nullptr)
        static_cast<MaterialPBRStandard *>(material)->setRoughnessTexture(roughness);

    return material;
}

Material *VulkanMaterials::createZipMaterial(const AssetInfo &info, Textures &textures)
{
    struct zip_t *zip = zip_open(info.filepath.c_str(), 0, 'r');
    if (zip == nullptr) {
        debug_tools::ConsoleWarning("VulkanMaterials()::createZipMaterial: Unable to open: " + info.filepath);
    }

    /* Get .xtex file name */
    std::string xtexName;
    std::string texturesFolder = "";
    int i, n = zip_entries_total(zip);
    for (i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        {
            const std::string name = std::string(zip_entry_name(zip));
            auto pointPos = name.find(".");
            if (pointPos != std::string::npos && name.substr(pointPos) == ".xtex") {
                xtexName = name;

                auto internalFolderIndex = name.rfind("/");
                if (internalFolderIndex != std::string::npos) {
                    texturesFolder = name.substr(0, internalFolderIndex) + "/";
                }
                break;
            }
        }
        zip_entry_close(zip);
    }

    void *buf = NULL;
    size_t bufsize;

    /* Parse .xtex file paremeters */
    glm::vec2 uv(1.F);
    if (zip_entry_open(zip, xtexName.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        std::string testT((char *)buf);

        const std::string X = testT.substr(0, testT.find("</width>")).substr(testT.find("<width>") + std::string("<width>").length());
        const std::string Y =
            testT.substr(0, testT.find("</height>")).substr(testT.find("<height>") + std::string("<height>").length());

        uv *= glm::vec2(100.F / std::stof(X), 100.F / std::stof(Y));
        zip_entry_close(zip);
    }

    std::string filename = getFilename(info.filepath);

    /* Parse albedo */
    Texture *albedoTexture = nullptr;
    std::string albedoZipPath = texturesFolder + "textures/albedo.png";
    if (zip_entry_open(zip, albedoZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer =
            stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, STBI_rgb_alpha);

        std::string id = filename + ":" + albedoZipPath;
        Image<stbi_uc> image(AssetInfo(id, info.filepath, info.source, AssetLocation::EMBEDDED),
                             rawImgBuffer,
                             x,
                             y,
                             STBI_rgb_alpha,
                             ColorSpace::sRGB,
                             true);
        albedoTexture = textures.createTexture(image);
        if (albedoTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load albedo from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse roughness */
    Texture *roughnessTexture = nullptr;
    std::string roughnessZipPath = texturesFolder + "textures/roughness.png";
    if (zip_entry_open(zip, roughnessZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y, channels;
        stbi_uc *rawImgBuffer =
            stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, &channels, STBI_default);

        std::string id = filename + ":" + roughnessZipPath;
        Image<stbi_uc> image(AssetInfo(id, info.filepath, info.source, AssetLocation::EMBEDDED),
                             rawImgBuffer,
                             x,
                             y,
                             channels,
                             ColorSpace::LINEAR,
                             true);
        roughnessTexture = textures.createTexture(image);
        if (roughnessTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load roughness from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    /* Parse normal */
    Texture *normalTexture = nullptr;
    std::string normalZipPath = texturesFolder + "textures/normal.png";
    if (zip_entry_open(zip, normalZipPath.c_str()) == 0) {
        zip_entry_read(zip, &buf, &bufsize);

        int32_t x, y;
        stbi_uc *rawImgBuffer =
            stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(buf), bufsize, &x, &y, nullptr, STBI_rgb_alpha);

        std::string id = filename + ":" + normalZipPath;
        Image<stbi_uc> image(AssetInfo(id, info.filepath, info.source, AssetLocation::EMBEDDED),
                             rawImgBuffer,
                             x,
                             y,
                             STBI_rgb_alpha,
                             ColorSpace::LINEAR,
                             true);
        normalTexture = textures.createTexture(image);
        if (normalTexture == nullptr) {
            debug_tools::ConsoleWarning("Unable to load normal from zip texture stack");
        }
        /* entry_close will delete the memory */
        zip_entry_close(zip);
    }

    auto mat = dynamic_cast<MaterialPBRStandard *>(createMaterial(info, MaterialType::MATERIAL_PBR_STANDARD));
    mat->setAlbedoTexture(albedoTexture);
    mat->setRoughnessTexture(roughnessTexture);
    mat->roughness() = 1.0F;
    mat->setNormalTexture(normalTexture);
    mat->metallic() = 0.0F;
    mat->uTiling() = uv.x;
    mat->vTiling() = uv.y;
    mat->zipMaterial() = true;

    return mat;
}

std::vector<Material *> VulkanMaterials::createImportedMaterials(const std::vector<ImportedMaterial> &importedMaterials,
                                                                 Textures &textures)
{
    auto getTexture = [&](ImportedTexture tex) {
        if (tex.image != nullptr) {
            auto texture = textures.createTexture(*tex.image);
            return texture;
        }

        assert(!tex.name.empty());

        auto texture = AssetManager::getInstance().texturesMap().get(tex.name);
        return texture;
    };

    /* Create stack materials first */
    for (uint32_t i = 0; i < importedMaterials.size(); i++) {
        const ImportedMaterial &mat = importedMaterials[i];
        if (mat.type == ImportedMaterialType::STACK) {
            createZipMaterial(mat.info, textures);
        }
    }

    std::vector<Material *> materials;
    for (uint32_t i = 0; i < importedMaterials.size(); i++) {
        const ImportedMaterial &mat = importedMaterials[i];

        if (mat.type == ImportedMaterialType::EMBEDDED) {
            materials.push_back(nullptr);
        } else if (mat.type == ImportedMaterialType::STACK) {
            materials.push_back(AssetManager::getInstance().materialsMap().get(mat.info.name));
        } else if (mat.type == ImportedMaterialType::LAMBERT) {
            auto material = Materials::createMaterial<MaterialLambert>(mat.info);

            materials.push_back(material);

            material->albedo() = mat.albedo;
            if (mat.albedoTexture.has_value()) {
                material->setAlbedoTexture(getTexture(mat.albedoTexture.value()));
            }
            material->ao() = mat.ao;
            if (mat.aoTexture.has_value()) {
                material->setAOTexture(getTexture(mat.aoTexture.value()));
            }
            material->emissive() = glm::vec4(mat.emissiveColor, mat.emissiveStrength);
            if (mat.emissiveTexture.has_value()) {
                material->setEmissiveTexture(getTexture(mat.emissiveTexture.value()));
            }
            if (mat.normalTexture.has_value()) {
                material->setNormalTexture(getTexture(mat.normalTexture.value()));
            }
            if (mat.alphaTexture.has_value()) {
                material->setAlphaTexture(getTexture(mat.alphaTexture.value()));
            }
            material->setTransparent(mat.transparent);
        } else if (mat.type == ImportedMaterialType::PBR_STANDARD) {
            auto material = Materials::createMaterial<MaterialPBRStandard>(mat.info);
            materials.push_back(material);

            material->albedo() = mat.albedo;
            if (mat.albedoTexture.has_value()) {
                material->setAlbedoTexture(getTexture(mat.albedoTexture.value()));
            }
            material->roughness() = mat.roughness;
            if (mat.roughnessTexture.has_value()) {
                material->setRoughnessTexture(getTexture(mat.roughnessTexture.value()));
            }
            material->metallic() = mat.metallic;
            if (mat.metallicTexture.has_value()) {
                material->setMetallicTexture(getTexture(mat.metallicTexture.value()));
            }
            material->ao() = mat.ao;
            if (mat.aoTexture.has_value()) {
                material->setAOTexture(getTexture(mat.aoTexture.value()));
            }
            material->emissive() = glm::vec4(mat.emissiveColor, mat.emissiveStrength);
            if (mat.emissiveTexture.has_value()) {
                material->setEmissiveTexture(getTexture(mat.emissiveTexture.value()));
            }
            if (mat.normalTexture.has_value()) {
                material->setNormalTexture(getTexture(mat.normalTexture.value()));
            }
            if (mat.alphaTexture.has_value()) {
                material->setAlphaTexture(getTexture(mat.alphaTexture.value()));
            }
            material->setTransparent(mat.transparent);
        } else if (mat.type == ImportedMaterialType::VOLUME) {
            auto material = Materials::createMaterial<MaterialVolume>(mat.info);
            materials.push_back(material);

            material->sigmaS() = glm::vec4(mat.sigmaS, 1.0F);
            material->sigmaA() = glm::vec4(mat.sigmaA, 1.0F);
            material->g() = mat.g;
        }
    }

    return materials;
}

VkResult VulkanMaterials::createDescriptorSetsLayout()
{
    /* Create binding for material data */
    VkDescriptorSetLayoutBinding materiaDatalLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                               VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                                           0,
                                           1);

    std::array<VkDescriptorSetLayoutBinding, 1> setBindings = {materiaDatalLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(setBindings.size()), setBindings.data());

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanMaterials::createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps)
{
    VkDescriptorPoolSize materialDataPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(nImages));
    VkDescriptorPoolSize cubemapDataPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(6 * maxCubemaps * nImages));
    std::array<VkDescriptorPoolSize, 2> poolSizes = {materialDataPoolSize, cubemapDataPoolSize};

    VkDescriptorPoolCreateInfo poolInfo = vkinit::descriptorPoolCreateInfo(
        static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), static_cast<uint32_t>(nImages + maxCubemaps * nImages));
    poolInfo.maxSets = static_cast<uint32_t>(nImages + maxCubemaps * nImages);

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanMaterials::createDescriptorSets(uint32_t nImages)
{
    m_descriptorSets.resize(nImages);

    std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSets.data()));

    return VK_SUCCESS;
}

void VulkanMaterials::updateDescriptor(uint32_t index)
{
    VkDescriptorBufferInfo bufferInfo = vkinit::descriptorBufferInfo(m_materialsStorage.buffer(index), 0, VK_WHOLE_SIZE);
    VkWriteDescriptorSet descriptorWrite =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfo);

    std::array<VkWriteDescriptorSet, 1> writeSets = {descriptorWrite};
    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

void VulkanMaterials::updateDescriptorSets()
{
    for (uint32_t i = 0; i < m_descriptorSets.size(); i++) {
        updateDescriptor(i);
    }
}

}  // namespace vengine