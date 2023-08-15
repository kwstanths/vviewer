#ifndef __DialogSceneExport_hpp__
#define __DialogSceneExport_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qradiobutton.h>

#include "UI/widgets/WidgetColorButton.hpp"

/* A dialog to export a scene */
class DialogSceneExport : public QDialog
{
    Q_OBJECT
public:
    DialogSceneExport(QWidget* parent);

    std::string getDestinationFolderName() const;
    std::string getFileType() const;
    uint32_t getResolutionWidth() const;
    uint32_t getResolutionHeight() const;
    uint32_t getSamples() const;
    uint32_t getDepth() const;
    uint32_t getRDepth() const;
    bool getBackground() const;
    glm::vec3 getBackgroundColor() const;
    float getTessellation() const;
    bool getParallax() const;
    float getFocalLength() const;
    uint32_t getApertureSides() const;
    float getAperture() const;

private:
    QLineEdit * m_widgetDestinationFolder = nullptr;
    QPushButton* m_buttonDestinationFolderChange = nullptr;
    std::string m_destinationFolderName = "";

    QStringList m_renderFileTypes = {"EXR", "PNG", "HDR", "JPEG"};
    QComboBox * m_widgetRenderFileTypes = nullptr;

    QSpinBox* m_widgetResolutionWidth = nullptr;
    QSpinBox* m_widgetResolutionHeight = nullptr;

    QSpinBox* m_widgetSamples = nullptr;
    QSpinBox* m_widgetDepth = nullptr;
    QSpinBox* m_widgetRDepth = nullptr;

    QRadioButton* m_widgetHideBackground = nullptr;
    WidgetColorButton* m_buttonBackgroundColor = nullptr;
    glm::vec3 m_backgroundColor;
    QDoubleSpinBox* m_widgetTessellation = nullptr;
    QRadioButton* m_widgetParallax = nullptr;

    QDoubleSpinBox* m_widgetFocalLength = nullptr;
    QSpinBox* m_widgetApertureSides = nullptr;
    QDoubleSpinBox* m_widgetAperture = nullptr;

    QPushButton* m_buttonOk = nullptr;
    QPushButton* m_buttonCancel = nullptr;

private Q_SLOTS:
    void onButtonSetDestinationFolder();
    void onLightColorChanged(glm::vec3);
    void onButtonOk();
    void onButtonCancel();
};

#endif