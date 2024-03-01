#ifndef __UIUtils_hpp__
#define __UIUtils_hpp__

#include <qstringlist.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qcombobox.h>

#include <vengine/core/Texture.hpp>

QStringList getImportedModels();
QStringList getCreatedMaterials();
QStringList getCreatedLights();
QStringList getImportedTextures(vengine::ColorSpace colorSpace);
QStringList getImportedCubemaps();
QStringList getImportedEnvironmentMaps();

void setButtonColor(QPushButton *button, QColor color);

/**
 * @brief Set combo box available options to the list of names, and set current option to currentText
 *
 * @param box
 * @param names
 * @param currentText
 * @param blockSignals
 */
void updateTextureComboBox(QComboBox *box, QStringList names, QString currentText, bool blockSignals = true);

#endif