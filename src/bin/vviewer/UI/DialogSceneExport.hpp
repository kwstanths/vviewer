#ifndef __DialogSceneExport_hpp__
#define __DialogSceneExport_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qspinbox.h>

/* A dialog to export a scene */
class DialogSceneExport : public QDialog
{
    Q_OBJECT
public:
    DialogSceneExport(QWidget* parent);

    std::string getDestinationFolderName() const;
    uint32_t getResolutionWidth() const;
    uint32_t getResolutionHeight() const;
    uint32_t getSamples() const;

private:
    QLineEdit * m_destinationFolderWidget = nullptr;
    QPushButton* m_destinationFolderChangeButton = nullptr;
    std::string m_destinationFolderName = "";

    QSpinBox* m_resolutionWidth = nullptr;
    QSpinBox* m_resolutionHeight = nullptr;
    QSpinBox* m_samples = nullptr;

    QPushButton* m_buttonOk = nullptr;
    QPushButton* m_buttonCancel = nullptr;

private slots:
    void onButtonSetDestinationFolder();
    void onButtonOk();
    void onButtonCancel();
};

#endif