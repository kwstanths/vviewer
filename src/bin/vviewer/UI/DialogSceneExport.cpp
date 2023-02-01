#include "DialogSceneExport.hpp"

#include "qfiledialog.h"
#include "qlayout.h"
#include "qlabel.h"
#include "qgroupbox.h"

DialogSceneExport::DialogSceneExport(QWidget* parent)
{
	setWindowTitle(QString("Export scene:"));

	/* Output folder */
	m_widgetDestinationFolder = new QLineEdit("./testScene");
	m_widgetDestinationFolder->setReadOnly(true);

	m_buttonDestinationFolderChange = new QPushButton("Change");
	connect(m_buttonDestinationFolderChange, &QPushButton::clicked, this, &DialogSceneExport::onButtonSetDestinationFolder);

	QHBoxLayout* layoutFolderName = new QHBoxLayout();
	layoutFolderName->addWidget(new QLabel("Folder: "));
	layoutFolderName->addWidget(m_widgetDestinationFolder);
	layoutFolderName->addWidget(m_buttonDestinationFolderChange);
	layoutFolderName->setContentsMargins(0, 0, 0, 0);
	QWidget* widgetFolderName = new QWidget();
	widgetFolderName->setLayout(layoutFolderName);

	/* Render params */
	/* Resolution, file type */
	m_widgetResolutionWidth = new QSpinBox();
	m_widgetResolutionWidth->setMinimum(0);
	m_widgetResolutionWidth->setMaximum(16384);
	m_widgetResolutionWidth->setValue(512);
	QHBoxLayout* layoutResolutionWidth = new QHBoxLayout();
	layoutResolutionWidth->addWidget(new QLabel("Width:"));
	layoutResolutionWidth->addWidget(m_widgetResolutionWidth);
	QWidget* widgetResolutionWidth = new QWidget();
	widgetResolutionWidth->setLayout(layoutResolutionWidth);

	m_widgetResolutionHeight = new QSpinBox();
	m_widgetResolutionHeight->setMinimum(0);
	m_widgetResolutionHeight->setMaximum(16384);
	m_widgetResolutionHeight->setValue(512);
	QHBoxLayout* layoutResolutionHeight = new QHBoxLayout();
	layoutResolutionHeight->addWidget(new QLabel("Height:"));
	layoutResolutionHeight->addWidget(m_widgetResolutionHeight);
	QWidget* widgetResolutionHeight = new QWidget();
	widgetResolutionHeight->setLayout(layoutResolutionHeight);

	m_widgetRenderFileTypes = new QComboBox();
	m_widgetRenderFileTypes->addItems(m_renderFileTypes);
	QHBoxLayout* layoutRenderFileType = new QHBoxLayout();
	layoutRenderFileType->addWidget(new QLabel("Output: "));
	layoutRenderFileType->addWidget(m_widgetRenderFileTypes);
	layoutRenderFileType->setAlignment(Qt::AlignLeft);
	QWidget * widgetRenderFileType = new QWidget();
	widgetRenderFileType->setLayout(layoutRenderFileType);

	QHBoxLayout * layoutResolution = new QHBoxLayout();
	layoutResolution->addWidget(widgetResolutionWidth);
	layoutResolution->addWidget(widgetResolutionHeight);
	layoutResolution->setAlignment(Qt::AlignLeft);
	layoutResolution->setContentsMargins(0, 0, 0, 0);
	QWidget * widgetResolution = new QWidget();
	widgetResolution->setLayout(layoutResolution);

	/* Samples, depth, rdepth */
	m_widgetSamples = new QSpinBox();
	m_widgetSamples->setMinimum(0);
	m_widgetSamples->setMaximum(65536);
	m_widgetSamples->setValue(64);
	QHBoxLayout* layoutSamples = new QHBoxLayout();
	layoutSamples->addWidget(new QLabel("Samples:"));
	layoutSamples->addWidget(m_widgetSamples);
	QWidget* widgetSamples = new QWidget();
	widgetSamples->setLayout(layoutSamples);

	m_widgetDepth = new QSpinBox();
	m_widgetDepth->setMinimum(1);
	m_widgetDepth->setMaximum(256);
	m_widgetDepth->setValue(11);
	QHBoxLayout* layoutDepth = new QHBoxLayout();
	layoutDepth->addWidget(new QLabel("Depth:"));
	layoutDepth->addWidget(m_widgetDepth);
	QWidget* widgetDepth = new QWidget();
	widgetDepth->setLayout(layoutDepth);

	m_widgetRDepth = new QSpinBox();
	m_widgetRDepth->setMinimum(1);
	m_widgetRDepth->setMaximum(256);
	m_widgetRDepth->setValue(3);
	QHBoxLayout* layoutRDepth = new QHBoxLayout();
	layoutRDepth->addWidget(new QLabel("Roulette depth:"));
	layoutRDepth->addWidget(m_widgetRDepth);
	QWidget* widgetRDepth = new QWidget();
	widgetRDepth->setLayout(layoutRDepth);

	QHBoxLayout * layoutTracingParams = new QHBoxLayout();
	layoutTracingParams->addWidget(widgetSamples);
	layoutTracingParams->addWidget(widgetDepth);
	layoutTracingParams->addWidget(widgetRDepth);
	layoutTracingParams->setAlignment(Qt::AlignLeft);
	layoutTracingParams->setContentsMargins(0, 0, 0, 0);
	QWidget * widgetTracingParams = new QWidget();
	widgetTracingParams->setLayout(layoutTracingParams);

	/* Render parameters group box */
	QGroupBox* renderParamsGroupBox = new QGroupBox("Render parameters");
	QVBoxLayout* layoutRenderParams = new QVBoxLayout();
	layoutRenderParams->addWidget(widgetRenderFileType);
	layoutRenderParams->addWidget(widgetResolution);
	layoutRenderParams->addWidget(widgetTracingParams);
	layoutRenderParams->setContentsMargins(0, 0, 0, 0);
	renderParamsGroupBox->setLayout(layoutRenderParams);

	/* Render flags */
	/* Background, tessellation, parallax */
	m_widgetHideBackground = new QRadioButton("Hide background");
	QHBoxLayout * layoutHideBackground = new QHBoxLayout();
	layoutHideBackground->addWidget(m_widgetHideBackground);
	QWidget * widgetHideBackground = new QWidget();
	widgetHideBackground->setLayout(layoutHideBackground);

	m_buttonBackgroundColor = new WidgetColorButton(this, {0, 0, 0});
	m_backgroundColor = {0, 0, 0};
    connect(m_buttonBackgroundColor, SIGNAL(colorChanged(glm::vec3)), this, SLOT(onLightColorChanged(glm::vec3)));
	QHBoxLayout * layoutBackgroundColor = new QHBoxLayout();
	layoutBackgroundColor->addWidget(new QLabel("Background "));
	layoutBackgroundColor->addWidget(m_buttonBackgroundColor);
	QWidget * widgetBackgroundColor = new QWidget();
	widgetBackgroundColor->setLayout(layoutBackgroundColor);

	m_widgetTessellation = new QDoubleSpinBox();
	m_widgetTessellation->setMinimum(0);
	m_widgetTessellation->setMaximum(4);
	m_widgetTessellation->setSingleStep(0.1);
	m_widgetTessellation->setValue(0);
	QHBoxLayout* layoutTessellation = new QHBoxLayout();
	layoutTessellation->addWidget(new QLabel("Tessellation:"));
	layoutTessellation->addWidget(m_widgetTessellation);
	layoutTessellation->setAlignment(Qt::AlignLeft);
	QWidget* widgetTessellation = new QWidget();
	widgetTessellation->setLayout(layoutTessellation);

	QHBoxLayout *layoutBackground = new QHBoxLayout();
	layoutBackground->addWidget(widgetHideBackground);
	layoutBackground->addWidget(widgetBackgroundColor);
	layoutBackground->setAlignment(Qt::AlignLeft);
	layoutBackground->setContentsMargins(0, 0, 0, 0);
	QWidget * widgetBackground = new QWidget();
	widgetBackground->setLayout(layoutBackground);

	m_widgetParallax = new QRadioButton("Enable parallax");
	QHBoxLayout * layoutParallax = new QHBoxLayout();
	layoutParallax->addWidget(m_widgetParallax);
	QWidget * widgetParallax = new QWidget();
	widgetParallax->setLayout(layoutParallax);

	/* Render flags group box */
	QGroupBox* renderParamsFlagsBox = new QGroupBox("Render flags");
	QVBoxLayout* layoutRenderFlags = new QVBoxLayout();
	layoutRenderFlags->addWidget(widgetBackground);
	layoutRenderFlags->addWidget(widgetTessellation);
	layoutRenderFlags->addWidget(widgetParallax);
	layoutRenderFlags->setContentsMargins(0, 0, 0, 0);
	renderParamsFlagsBox->setLayout(layoutRenderFlags);

	/* Camera parameters */
	/* Aperture, focal length */
	m_widgetFocalLength = new QDoubleSpinBox();
	m_widgetFocalLength->setMinimum(1);
	m_widgetFocalLength->setMaximum(100);
	m_widgetFocalLength->setSingleStep(0.1);
	m_widgetFocalLength->setValue(10);
	QHBoxLayout* layoutFocalLength = new QHBoxLayout();
	layoutFocalLength->addWidget(new QLabel("Focal length:"));
	layoutFocalLength->addWidget(m_widgetFocalLength);
	layoutFocalLength->setAlignment(Qt::AlignLeft);
	QWidget* widgetFocalLength = new QWidget();
	widgetFocalLength->setLayout(layoutFocalLength);

	m_widgetApertureSides = new QSpinBox();
	m_widgetApertureSides->setMinimum(1);
	m_widgetApertureSides->setMaximum(10);
	m_widgetApertureSides->setValue(3);
	QHBoxLayout* layoutApertureSides = new QHBoxLayout();
	layoutApertureSides->addWidget(new QLabel("Aperture sides:"));
	layoutApertureSides->addWidget(m_widgetApertureSides);
	layoutApertureSides->setAlignment(Qt::AlignLeft);
	QWidget* widgetApertureSides = new QWidget();
	widgetApertureSides->setLayout(layoutApertureSides);
	
	m_widgetAperture = new QDoubleSpinBox();
	m_widgetAperture->setMinimum(1);
	m_widgetAperture->setMaximum(50);
	m_widgetAperture->setValue(10);
	QHBoxLayout* layoutAperture = new QHBoxLayout();
	layoutAperture->addWidget(new QLabel("Aperture:"));
	layoutAperture->addWidget(m_widgetAperture);
	layoutAperture->addWidget(new QLabel("mm"));
	layoutAperture->setAlignment(Qt::AlignLeft);
	QWidget* widgetAperture = new QWidget();
	widgetAperture->setLayout(layoutAperture);

	/* Camera parameters group box */
	QGroupBox* cameraParamsBox = new QGroupBox("Camera parameters");
	QVBoxLayout* layoutCameraParams = new QVBoxLayout();
	layoutCameraParams->addWidget(widgetFocalLength);
	layoutCameraParams->addWidget(widgetApertureSides);
	layoutCameraParams->addWidget(widgetAperture);
	layoutCameraParams->setContentsMargins(0, 0, 0, 0);
	cameraParamsBox->setLayout(layoutCameraParams);

	/* Buttons */
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
	layoutMain->addWidget(renderParamsGroupBox);
	layoutMain->addWidget(renderParamsFlagsBox);
	layoutMain->addWidget(cameraParamsBox);
	layoutMain->addWidget(widgetButtons);

	setLayout(layoutMain);

	this->setFixedHeight(650);
	this->setFixedWidth(500);
}

std::string DialogSceneExport::getDestinationFolderName() const
{
	return m_destinationFolderName;
}

std::string DialogSceneExport::getFileType() const
{
	return m_widgetRenderFileTypes->currentText().toStdString();
}

uint32_t DialogSceneExport::getResolutionWidth() const
{
	return m_widgetResolutionWidth->value();
}

uint32_t DialogSceneExport::getResolutionHeight() const
{
	return m_widgetResolutionHeight->value();
}

uint32_t DialogSceneExport::getSamples() const
{
	return m_widgetSamples->value();
}

uint32_t DialogSceneExport::getDepth() const
{
	return m_widgetDepth->value();
}

uint32_t DialogSceneExport::getRDepth() const
{
	return m_widgetRDepth->value();
}

bool DialogSceneExport::getBackground() const
{
	return m_widgetHideBackground->isChecked();
}

glm::vec3 DialogSceneExport::getBackgroundColor() const
{
	return m_backgroundColor;
}

float DialogSceneExport::getTessellation() const
{
	return m_widgetTessellation->value();
}

bool DialogSceneExport::getParallax() const
{
	return m_widgetParallax->isChecked();
}

float DialogSceneExport::getFocalLength() const
{
	return m_widgetFocalLength->value();
}

uint32_t DialogSceneExport::getApertureSides() const
{
	return m_widgetApertureSides->value();
}

float DialogSceneExport::getAperture() const
{
	return m_widgetAperture->value();
}

void DialogSceneExport::onButtonSetDestinationFolder()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Choose directory", "./assets/scenes/", QFileDialog::ShowDirsOnly);
	if (dir == "") return;
	
	m_widgetDestinationFolder->setText(dir);
}

void DialogSceneExport::onLightColorChanged(glm::vec3 color)
{
	m_backgroundColor = color;
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
