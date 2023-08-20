#ifndef __WidgetSceneGraph_hpp__
#define __WidgetSceneGraph_hpp__

#include <memory>

#include <qtreewidget.h>
#include <qwidget.h>
#include <qtmetamacros.h>

#include "core/SceneObject.hpp"

Q_DECLARE_METATYPE(std::shared_ptr<vengine::SceneObject>)

class WidgetSceneGraph : public QTreeWidget
{
    Q_OBJECT
public:
    WidgetSceneGraph(QWidget * parent);

    QTreeWidgetItem * getPreviouslySelectedItem() const;
    void setPreviouslySelectedItem(QTreeWidgetItem * item);

    static std::shared_ptr<vengine::SceneObject> getSceneObject(QTreeWidgetItem* item)
    {
        return item->data(0, Qt::UserRole).value<std::shared_ptr<vengine::SceneObject>>();
    }

    QTreeWidgetItem* createTreeWidgetItem(std::shared_ptr<vengine::SceneObject> object);

    void removeItem(QTreeWidgetItem * item);

    QTreeWidgetItem * getTreeWidgetItem(std::shared_ptr<vengine::SceneObject> object);

private:
    /* Previously selected item */
    QTreeWidgetItem * m_previouslySelectedItem = nullptr;
    /* Holds a relationships between IDs and tree widget items */
    std::unordered_map<vengine::ID, QTreeWidgetItem*> m_treeWidgetItems;

private:
    void mouseReleaseEvent(QMouseEvent *ev) override;

};

#endif