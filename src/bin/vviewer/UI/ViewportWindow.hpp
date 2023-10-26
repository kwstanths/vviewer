#ifndef __ViewportWindow_hpp__
#define __ViewportWindow_hpp__

#include <qwindow.h>
#include <qvulkaninstance.h>
#include <qtimer.h>

#include "vengine/core/Engine.hpp"

class ViewportWindow : public QWindow
{
    Q_OBJECT
public:
    ViewportWindow()
        : QWindow(){};

    ~ViewportWindow(){};

    virtual vengine::Engine *engine() const = 0;

    virtual void windowActicated(bool activated) = 0;

Q_SIGNALS:
    /* emitted when the initialization has finished */
    void initializationFinished();

    /* emitted when the user selected an object from the 3d scene */
    void sceneObjectSelected(std::shared_ptr<vengine::SceneObject> object);

    /* emitted when the position of the selected object changes */
    void selectedObjectPositionChanged();

private:
};

#endif