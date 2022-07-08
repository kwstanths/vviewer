#include "IDGeneration.hpp"

#include "Console.hpp"

ID IDGeneration::getID()
{
	return m_index++;
}

glm::vec3 IDGeneration::toRGB(ID id)
{
	if (id > 0xFFFFFF) {
		utils::ConsoleWarning("IdGeneration::toRGB(): ID encoding overflow");
	}

	glm::vec3 rgb;
	rgb.r = (id & 0xFF) / 255.0F;
	rgb.g = ((id >> 8) & 0xFF) / 255.0F;
	rgb.b = ((id >> 16) & 0xFF) / 255.0F;
	return rgb;
}

ID IDGeneration::fromRGB(glm::vec3 rgb)
{
	return static_cast<uint32_t>(rgb.r * 255.0F) |
		(static_cast<uint32_t>(rgb.g * 255.0F) << 8) |
		(static_cast<uint32_t>(rgb.b * 255.0F) << 16);
}
