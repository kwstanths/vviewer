#ifndef __AssetManager_hpp__
#define __AssetManager_hpp__

#include <unordered_map>
#include <stdexcept>

/**
    A singleton class that wraps an unordered map to store an AssetID -> Asset relationship
*/
template<typename AssetID, typename Asset>
class AssetManager {
public:
    static AssetManager& getInstance()
    {
        static AssetManager instance;
        return instance;
    }
    AssetManager(AssetManager const&) = delete;
    void operator=(AssetManager const&) = delete;

    typedef typename std::unordered_map<AssetID, Asset>::iterator Iterator;

    bool isPresent(AssetID id) {
        if (m_assets.find(id) == m_assets.end()) {
            return false;
        }
        return true;
    }

    void add(AssetID id, Asset asset) {
        m_assets[id] = asset;
    }

    Asset get(AssetID id) {
        auto itr = m_assets.find(id);
        if (itr != m_assets.end()) {
            return itr->second;
        }
        throw std::runtime_error("Find: Asset not found: " + id);
    }

    Iterator remove(AssetID id) {
        auto itr = m_assets.find(id);
        if (itr == m_assets.end()){
            throw std::runtime_error("Erase: Asset not found: " + id);
        }

        return m_assets.erase(itr);
    }

    void reset() {
        m_assets.clear();
    }

    Iterator begin() {
        return m_assets.begin();
    }

    Iterator end() {
        return m_assets.end();
    }

private:
    AssetManager() {}

    std::unordered_map<AssetID, Asset> m_assets;
};

#endif
