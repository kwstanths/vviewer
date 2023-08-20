#include "WidgetSceneGraph.hpp"

#include <QMouseEvent>
#include <cstddef>
#include <qabstractitemmodel.h>
#include <qpoint.h>

using namespace vengine;

WidgetSceneGraph::WidgetSceneGraph(QWidget * parent) : QTreeWidget(parent)
{

}

QTreeWidgetItem * WidgetSceneGraph::getPreviouslySelectedItem() const
{
    return m_previouslySelectedItem;
}

void WidgetSceneGraph::setPreviouslySelectedItem(QTreeWidgetItem * item)
{
    m_previouslySelectedItem = item;
}

QTreeWidgetItem* WidgetSceneGraph::createTreeWidgetItem(std::shared_ptr<SceneObject> object)
{
    QTreeWidgetItem* item = new QTreeWidgetItem({ QString(object->m_name.c_str()) });
    QVariant data;
    data.setValue(object);
    item->setData(0, Qt::UserRole, data);
    m_treeWidgetItems.insert({ object->getID(), item });
    return item;
}

void WidgetSceneGraph::removeItem(QTreeWidgetItem * item)
{
    m_previouslySelectedItem = nullptr;

    /* Remove from m_treeWidgetItems recursively */
    std::function<void(QTreeWidgetItem*)> removeChild;
    removeChild = [&](QTreeWidgetItem* child) { 
        std::shared_ptr<SceneObject> object = WidgetSceneGraph::getSceneObject(child);
        m_treeWidgetItems.erase(object->getID());
        for(size_t i=0; i < child->childCount(); i++) {
            removeChild(child->child(i));
        }    
    };

    /* Remove from map */
    m_treeWidgetItems.erase(WidgetSceneGraph::getSceneObject(item)->getID());
    /* Do the same for all children recurisvely */
    for(size_t i=0; i < item->childCount(); i++) {
        removeChild(item->child(i));
    }

    /* Delete and remove from UI */
    delete item;
}

QTreeWidgetItem * WidgetSceneGraph::getTreeWidgetItem(std::shared_ptr<SceneObject> object)
{
    auto itr = m_treeWidgetItems.find(object->getID());
    if (itr == m_treeWidgetItems.end()) return nullptr;

    return itr->second;
}

void WidgetSceneGraph::mouseReleaseEvent(QMouseEvent *ev) 
{
    QPointF pos = ev->pos();
    QModelIndex index = indexAt(pos.toPoint());
    if(!index.isValid())
    {
        clearSelection();
        // clearSelection doesn't clear the curentItem() function call that still returns the previous selected
        setCurrentIndex(QModelIndex());
    }
}