#ifndef __WidgetMeshModel_hpp__
#define __WidgetMeshModel_hpp__

#include <qwidget.h>
#include <qcombobox.h>

/* A UI widget to represent a mesh model */
class WidgetMeshModel : public QWidget
{
    Q_OBJECT
public:
    WidgetMeshModel(QWidget * parent, QStringList availableModels);

    std::string getSelectedModel() const;

    QComboBox * m_models = nullptr;
private:

};

#endif