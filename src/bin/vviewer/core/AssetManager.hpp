#ifndef __AssetManager_hpp__
#define __AssetManager_hpp__

#include <unordered_map>

template<typename AssetID, typename Asset>
class AssetManager {
public:
    typedef typename std::unordered_map<AssetID, Asset>::iterator Iterator;

    bool isPresent(AssetID id) {
        if (m_assets.find(id) == m_assets.end()) {
            return false;
        }
        return true;
    }

    void Add(AssetID id, Asset asset) {
        m_assets[id] = asset;
    }

    Asset Get(AssetID id) {
        return m_assets[id];
    }

    Iterator begin() {
        return m_assets.begin();
    }

    Iterator end() {
        return m_assets.end();
    }

private:
    std::unordered_map<AssetID, Asset> m_assets;
};

#endif
