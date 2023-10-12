#include "DialogSceneExport.hpp"

#include "qfiledialog.h"
#include "qlayout.h"
#include "qlabel.h"
#include "qgroupbox.h"

DialogSceneExport::DialogSceneExport(QWidget *parent)
{
    setWindowTitle(QString("Export scene:"));

    /* Output folder */
    m_widgetDestinationFolder = new QLineEdit("./testScene");
    m_widgetDestinationFolder->setReadOnly(true);

    m_buttonDestinationFolderChange = new QPushButton("...");
    connect(m_buttonDestinationFolderChange, &QPushButton::clicked, this, &DialogSceneExport::onButtonSetDestinationFolder);

    QHBoxLayout *layoutFolderName = new QHBoxLayout();
    layoutFolderName->addWidget(new QLabel("Folder: "));
    layoutFolderName->addWidget(m_widgetDestinationFolder);
    layoutFolderName->addWidget(m_buttonDestinationFolderChange);
    layoutFolderName->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetFolderName = new QWidget();
    widgetFolderName->setLayout(layoutFolderName);

    /* Buttons */
    m_buttonOk = new QPushButton(tr("Ok"));
    connect(m_buttonOk, &QPushButton::released, this, &DialogSceneExport::onButtonOk);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogSceneExport::onButtonCancel);
    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonOk);
    QWidget *widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widgetFolderName);
    layoutMain->addWidget(widgetButtons);

    setLayout(layoutMain);

    this->setFixedHeight(120);
    this->setFixedWidth(500);
}

std::string DialogSceneExport::getDestinationFolderName() const
{
    return m_destinationFolderName;
}

void DialogSceneExport::onButtonSetDestinationFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/scenes/", QFileDialog::ShowDirsOnly);
    if (dir == "")
        return;

    m_widgetDestinationFolder->setText(dir);
}

void DialogSceneExport::onButtonOk()
{
    m_destinationFolderName = m_widgetDestinationFolder->text().toStdString();

    close();
}

void DialogSceneExport::onButtonCancel()
{
    close();
}
