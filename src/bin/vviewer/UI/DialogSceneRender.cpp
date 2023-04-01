#include "DialogSceneRender.hpp"

#include "qfiledialog.h"
#include "qlayout.h"
#include "qlabel.h"
#include "qgroupbox.h"
#include "vulkan/renderers/VulkanRendererRayTracing.hpp"
#include <qcombobox.h>
#include <qlistwidget.h>
#include <sys/types.h>

DialogSceneRender::DialogSceneRender(QWidget* parent, const VulkanRendererRayTracing& renderer)
{
	setWindowTitle(QString("Render scene:"));

    /* Filename widget */
	m_renderOutputFileWidget = new QLineEdit(QString::fromStdString(renderer.getRenderOutputFileName()));

	m_renderOutputFileChangeButton = new QPushButton("Change");
	connect(m_renderOutputFileChangeButton, &QPushButton::clicked, this, &DialogSceneRender::onButtonSetRenderOutputFile);

    m_renderOutputFileTypeWidget = new QComboBox(this);
    m_renderOutputFileTypeWidget->addItem("PNG");
    m_renderOutputFileTypeWidget->addItem("HDR");
    m_renderOutputFileTypeWidget->setCurrentIndex(1);

	QHBoxLayout* layoutRenderOutputFile = new QHBoxLayout();
	layoutRenderOutputFile->addWidget(m_renderOutputFileWidget);
    layoutRenderOutputFile->addWidget(m_renderOutputFileTypeWidget);
	layoutRenderOutputFile->addWidget(m_renderOutputFileChangeButton);
	QWidget* widgetRenderOutputFile = new QWidget();
	widgetRenderOutputFile->setLayout(layoutRenderOutputFile);

    /* Resolution widget */
    uint32_t currentWidth;
    uint32_t currentHeight;
    renderer.getRenderResolution(currentWidth, currentHeight);
	m_resolutionWidth = new QSpinBox();
	m_resolutionWidth->setMinimum(0);
	m_resolutionWidth->setMaximum(16384);
	m_resolutionWidth->setValue(currentWidth);
    QHBoxLayout* layoutResolutionWidth = new QHBoxLayout();
	layoutResolutionWidth->addWidget(new QLabel("Width:"));
	layoutResolutionWidth->addWidget(m_resolutionWidth);
	QWidget* widgetResolutionWidth = new QWidget();
	widgetResolutionWidth->setLayout(layoutResolutionWidth);

	m_resolutionHeight = new QSpinBox();
	m_resolutionHeight->setMinimum(0);
	m_resolutionHeight->setMaximum(16384);
	m_resolutionHeight->setValue(currentHeight);
	QHBoxLayout* layoutResolutionHeight = new QHBoxLayout();
	layoutResolutionHeight->addWidget(new QLabel("Height:"));
	layoutResolutionHeight->addWidget(m_resolutionHeight);
	QWidget* widgetResolutionHeight = new QWidget();
	widgetResolutionHeight->setLayout(layoutResolutionHeight);

	QGroupBox* resolutionGroupBox = new QGroupBox("Resolution:");
	QHBoxLayout* layoutResolution = new QHBoxLayout();
	layoutResolution->addWidget(widgetResolutionWidth);
	layoutResolution->addWidget(widgetResolutionHeight);
	resolutionGroupBox->setLayout(layoutResolution);

    /* Render parameters widget */
	m_samples = new QSpinBox();
	m_samples->setMinimum(0);
	m_samples->setMaximum(65536);
	m_samples->setValue(renderer.getSamples());
	QHBoxLayout* layoutSamples = new QHBoxLayout();
	layoutSamples->addWidget(new QLabel("Samples:"));
	layoutSamples->addWidget(m_samples);
	QWidget* widgetSamples = new QWidget();
	widgetSamples->setLayout(layoutSamples);

    m_depth = new QSpinBox();
	m_depth->setMinimum(0);
	m_depth->setMaximum(65536);
	m_depth->setValue(renderer.getMaxDepth());
    QHBoxLayout* layoutDepth = new QHBoxLayout();
	layoutDepth->addWidget(new QLabel("Max depth:"));
	layoutDepth->addWidget(m_depth);
	QWidget* widgetDepth = new QWidget();
	widgetDepth->setLayout(layoutDepth);

	QGroupBox* renderGroupBox = new QGroupBox("Parameters");
	QHBoxLayout* layoutRender = new QHBoxLayout();
	layoutRender->addWidget(widgetSamples);
	layoutRender->addWidget(widgetDepth);
	renderGroupBox->setLayout(layoutRender);

    /* Buttons widget */
	m_buttonRender = new QPushButton(tr("Render"));
	connect(m_buttonRender, &QPushButton::released, this, &DialogSceneRender::onButtonRender);
	m_buttonCancel = new QPushButton(tr("Cancel"));
	connect(m_buttonCancel, &QPushButton::released, this, &DialogSceneRender::onButtonCancel);
	QHBoxLayout* layoutButtons = new QHBoxLayout();
	layoutButtons->addWidget(m_buttonCancel);
	layoutButtons->addWidget(m_buttonRender);
	QWidget* widgetButtons = new QWidget();
	widgetButtons->setLayout(layoutButtons);

    /* Main widget */
	QVBoxLayout* layoutMain = new QVBoxLayout();
	layoutMain->addWidget(widgetRenderOutputFile);
    layoutMain->addWidget(resolutionGroupBox);
	layoutMain->addWidget(renderGroupBox);
	layoutMain->addWidget(widgetButtons);
    layoutMain->setSpacing(10);

	setLayout(layoutMain);

	this->setMinimumWidth(300);
}

std::string DialogSceneRender::getRenderOutputFileName() const
{
	return m_renderOutputFileName;
}

OutputFileType DialogSceneRender::getRenderOutputFileType() const
{
    return static_cast<OutputFileType>(m_renderOutputFileTypeWidget->currentIndex());
}

uint32_t DialogSceneRender::getResolutionWidth() const
{
	return m_resolutionWidth->value();
}

uint32_t DialogSceneRender::getResolutionHeight() const
{
	return m_resolutionHeight->value();
}

uint32_t DialogSceneRender::getSamples() const
{
	return m_samples->value();
}

uint32_t DialogSceneRender::getDepth() const
{
	return m_depth->value();
}

void DialogSceneRender::onButtonSetRenderOutputFile()
{
	QString filename = QFileDialog::getSaveFileName(this, "Choose fle", "./");
	if (filename == "") return;
	
	m_renderOutputFileWidget->setText(filename);
}

void DialogSceneRender::onButtonRender()
{
	m_renderOutputFileName = m_renderOutputFileWidget->text().toStdString();

	close();
}

void DialogSceneRender::onButtonCancel()
{
	close();
}
