#ifndef __UIUtils_hpp__
#define __UIUtils_hpp__

#include <qstringlist.h>
#include <qwidget.h>
#include <qpushbutton.h>

#include <core/Texture.hpp>

QStringList getImportedModels();
QStringList getCreatedMaterials();
QStringList getImportedTextures(TextureType type);
QStringList getImportedCubemaps();
QStringList getImportedEnvironmentMaps();

QWidget * createColorWidget(QPushButton ** button);
void setButtonColor(QPushButton* button, QColor color);


#endif