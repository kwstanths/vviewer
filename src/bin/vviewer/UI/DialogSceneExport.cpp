#include "DialogSceneExport.hpp"

#include "qfiledialog.h"
#include "qlayout.h"
#include "qlabel.h"
#include "qgroupbox.h"

DialogSceneExport::DialogSceneExport(QWidget* parent)
{
	setWindowTitle(QString("Export scene:"));

	m_destinationFolderWidget = new QLineEdit("./testScene");
	m_destinationFolderWidget->setReadOnly(true);

	m_destinationFolderChangeButton = new QPushButton("Change");
	connect(m_destinationFolderChangeButton, &QPushButton::clicked, this, &DialogSceneExport::onButtonSetDestinationFolder);

	QHBoxLayout* layoutFolderName = new QHBoxLayout();
	layoutFolderName->addWidget(m_destinationFolderWidget);
	layoutFolderName->addWidget(m_destinationFolderChangeButton);
	QWidget* widgetFolderName = new QWidget();
	widgetFolderName->setLayout(layoutFolderName);

	m_resolutionWidth = new QSpinBox();
	m_resolutionWidth->setMinimum(0);
	m_resolutionWidth->setMaximum(16384);
	m_resolutionWidth->setValue(1024);
	m_resolutionHeight = new QSpinBox();
	m_resolutionHeight->setMinimum(0);
	m_resolutionHeight->setMaximum(16384);
	m_resolutionHeight->setValue(1024);
	m_samples = new QSpinBox();
	m_samples->setMinimum(0);
	m_samples->setMaximum(65536);
	m_samples->setValue(256);
	QHBoxLayout* layoutResolutionWidth = new QHBoxLayout();
	layoutResolutionWidth->addWidget(new QLabel("Width:"));
	layoutResolutionWidth->addWidget(m_resolutionWidth);
	QWidget* widgetResolutionWidth = new QWidget();
	widgetResolutionWidth->setLayout(layoutResolutionWidth);
	QHBoxLayout* layoutResolutionHeight = new QHBoxLayout();
	layoutResolutionHeight->addWidget(new QLabel("Height:"));
	layoutResolutionHeight->addWidget(m_resolutionHeight);
	QWidget* widgetResolutionHeight = new QWidget();
	widgetResolutionHeight->setLayout(layoutResolutionHeight);
	QHBoxLayout* layoutSamples = new QHBoxLayout();
	layoutSamples->addWidget(new QLabel("Samples:"));
	layoutSamples->addWidget(m_samples);
	QWidget* widgetSamples = new QWidget();
	widgetSamples->setLayout(layoutSamples);
	/* Render params group */
	QGroupBox* renderGroupBox = new QGroupBox("Render parameters");
	QHBoxLayout* layoutRender = new QHBoxLayout();
	layoutRender->addWidget(widgetResolutionWidth);
	layoutRender->addWidget(widgetResolutionHeight);
	layoutRender->addWidget(widgetSamples);
	renderGroupBox->setLayout(layoutRender);

	m_buttonOk = new QPushButton(tr("Ok"));
	connect(m_buttonOk, &QPushButton::released, this, &DialogSceneExport::onButtonOk);
	m_buttonCancel = new QPushButton(tr("Cancel"));
	connect(m_buttonCancel, &QPushButton::released, this, &DialogSceneExport::onButtonCancel);
	QHBoxLayout* layoutButtons = new QHBoxLayout();
	layoutButtons->addWidget(m_buttonCancel);
	layoutButtons->addWidget(m_buttonOk);
	QWidget* widgetButtons = new QWidget();
	widgetButtons->setLayout(layoutButtons);

	QVBoxLayout* layoutMain = new QVBoxLayout();
	layoutMain->addWidget(widgetFolderName);
	layoutMain->addWidget(renderGroupBox);
	layoutMain->addWidget(widgetButtons);

	setLayout(layoutMain);

	this->setMinimumWidth(400);
}

std::string DialogSceneExport::getDestinationFolderName() const
{
	return m_destinationFolderName;
}

uint32_t DialogSceneExport::getResolutionWidth() const
{
	return m_resolutionWidth->value();
}

uint32_t DialogSceneExport::getResolutionHeight() const
{
	return m_resolutionHeight->value();
}

uint32_t DialogSceneExport::getSamples() const
{
	return m_samples->value();
}

void DialogSceneExport::onButtonSetDestinationFolder()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/materials/", QFileDialog::ShowDirsOnly);
	if (dir == "") return;
	
	m_destinationFolderWidget->setText(dir);
}

void DialogSceneExport::onButtonOk()
{
	m_destinationFolderName = m_destinationFolderWidget->text().toStdString();

	close();
}

void DialogSceneExport::onButtonCancel()
{
	close();
}
