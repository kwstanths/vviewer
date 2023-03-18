#ifndef __DialogWaiting_hpp__
#define __DialogWaiting_hpp__

#include <functional>
#include <thread>

#include <qdialog.h>
#include <qstring.h>
#include <qtimer.h>
#include <qprogressbar.h>

#include "utils/Tasks.hpp"

/* A dialog to add an object in a scene */
class DialogWaiting : public QDialog
{
    Q_OBJECT
public:
    DialogWaiting(QWidget *parent, QString text, TaskWaitableUI * task);

private:
    QTimer * m_timer;
    std::thread m_thread;
    TaskWaitableUI * m_task;

    QProgressBar * m_progressBar;

    void closeEvent (QCloseEvent *event);
    
private slots:
    void update();
};

#endif