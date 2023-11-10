#ifndef __DialogCreateLight_hpp__
#define __DialogCreateLight_hpp__

#include <qdialog.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtextedit.h>

#include "vengine/core/Light.hpp"

/* A dialog to add an object in a scene */
class DialogCreateLight : public QDialog
{
    Q_OBJECT
public:
    DialogCreateLight(QWidget *parent, const char *name);

    std::optional<vengine::LightType> m_selectedLightType;
    QString m_selectedName = "";

private:
    QTextEdit *m_name;
    QComboBox *m_lightType;
    QPushButton *m_buttonOk = nullptr;
    QPushButton *m_buttonCancel = nullptr;

private Q_SLOTS:
    void onButtonOk();
    void onButtonCancel();
};

#endif