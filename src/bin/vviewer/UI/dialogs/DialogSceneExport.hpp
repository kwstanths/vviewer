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
    DialogSceneExport(QWidget *parent);

    std::string getDestinationFolderName() const;
    uint32_t getResolutionWidth() const;
    uint32_t getResolutionHeight() const;
    uint32_t getSamples() const;
    uint32_t getDepth() const;

private:
    QLineEdit *m_widgetDestinationFolder = nullptr;
    QPushButton *m_buttonDestinationFolderChange = nullptr;
    std::string m_destinationFolderName = "";

    QPushButton *m_buttonOk = nullptr;
    QPushButton *m_buttonCancel = nullptr;

private Q_SLOTS:
    void onButtonSetDestinationFolder();
    void onButtonOk();
    void onButtonCancel();
};

#endif