#ifndef __UIUtils_hpp__
#define __UIUtils_hpp__

#include <qstringlist.h>

#include <core/Texture.hpp>

QStringList getImportedModels();
QStringList getCreatedMaterials();
QStringList getImportedTextures(TextureType type);


#endif