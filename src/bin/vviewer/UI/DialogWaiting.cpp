#include "DialogWaiting.hpp"

#include "qlayout.h"
#include "qlabel.h"
#include "QCloseEvent"

DialogWaiting::DialogWaiting(QWidget *parent, QString text, Task * task) : m_task(task)
{
    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(text));
    layoutMain->addWidget(m_progressBar);

    setLayout(layoutMain);
    setFixedSize(150, 70);

    m_thread = std::thread([this]() {
        (*m_task)(); 
    });

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->start(100);
}

void DialogWaiting::closeEvent(QCloseEvent *event)
{
    if (!m_task->finished)
    {
        event->ignore();
    } else 
    {
        m_timer->stop();
        m_thread.join();
        event->accept();
    }
}

void DialogWaiting::update()
{
    m_progressBar->setValue(m_task->getProgress() * 100);
    if (!m_task->finished) return;

    close();
}

