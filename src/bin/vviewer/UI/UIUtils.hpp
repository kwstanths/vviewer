#ifndef __UIUtils_hpp__
#define __UIUtils_hpp__

#include <qstringlist.h>
#include <qwidget.h>
#include <qpushbutton.h>

#include <vengine/core/Texture.hpp>

QStringList getImportedModels();
QStringList getCreatedMaterials();
QStringList getCreatedLights();
QStringList getImportedTextures(vengine::ColorSpace colorSpace);
QStringList getImportedCubemaps();
QStringList getImportedEnvironmentMaps();

void setButtonColor(QPushButton *button, QColor color);

#endif