#ifndef __WidgetSceneGraph_hpp__
#define __WidgetSceneGraph_hpp__

#include <memory>

#include <qtreewidget.h>
#include <qwidget.h>
#include <qtmetamacros.h>

#include "vengine/core/SceneObject.hpp"

Q_DECLARE_METATYPE(vengine::SceneObject *)

class WidgetSceneGraph : public QTreeWidget
{
    Q_OBJECT
public:
    WidgetSceneGraph(QWidget *parent);

    QTreeWidgetItem *getPreviouslySelectedItem() const;
    void setPreviouslySelectedItem(QTreeWidgetItem *item);

    static vengine::SceneObject *getSceneObject(QTreeWidgetItem *item)
    {
        return item->data(0, Qt::UserRole).value<vengine::SceneObject *>();
    }

    QTreeWidgetItem *createTreeWidgetItem(vengine::SceneObject *object);

    void removeItem(QTreeWidgetItem *item);

    QTreeWidgetItem *getTreeWidgetItem(vengine::SceneObject *object);

    bool isEmpty() const;

private:
    /* Previously selected item */
    QTreeWidgetItem *m_previouslySelectedItem = nullptr;
    /* Holds a relationships between IDs and tree widget items */
    std::unordered_map<vengine::ID, QTreeWidgetItem *> m_treeWidgetItems;

private:
    void mouseReleaseEvent(QMouseEvent *ev) override;
};

#endif