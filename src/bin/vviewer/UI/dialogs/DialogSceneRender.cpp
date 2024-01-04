#include "DialogSceneRender.hpp"

#include "qfiledialog.h"
#include "qlayout.h"
#include "qlabel.h"
#include "qgroupbox.h"
#include <qcombobox.h>
#include <qlistwidget.h>
#include <sys/types.h>

DialogSceneRender::DialogSceneRender(QWidget *parent, const RendererPathTracing::RenderInfo &renderInfo)
{
    setWindowTitle(QString("Render scene:"));

    /* Filename widget */
    m_renderOutputFileWidget = new QLineEdit(QString::fromStdString(renderInfo.filename));

    m_renderOutputFileChangeButton = new QPushButton("Change");
    connect(m_renderOutputFileChangeButton, &QPushButton::clicked, this, &DialogSceneRender::onButtonSetRenderOutputFile);

    m_renderOutputFileTypeWidget = new QComboBox(this);
    m_renderOutputFileTypeWidget->addItem("png");
    m_renderOutputFileTypeWidget->addItem("hdr");
    m_renderOutputFileTypeWidget->setCurrentIndex(1);

    QHBoxLayout *layoutRenderOutputFile = new QHBoxLayout();
    layoutRenderOutputFile->addWidget(m_renderOutputFileWidget);
    layoutRenderOutputFile->addWidget(m_renderOutputFileTypeWidget);
    layoutRenderOutputFile->addWidget(m_renderOutputFileChangeButton);
    QWidget *widgetRenderOutputFile = new QWidget();
    widgetRenderOutputFile->setLayout(layoutRenderOutputFile);

    /* Resolution widget */
    uint32_t currentWidth = renderInfo.width;
    uint32_t currentHeight = renderInfo.height;
    m_resolutionWidth = new QSpinBox();
    m_resolutionWidth->setMinimum(0);
    m_resolutionWidth->setMaximum(16384);
    m_resolutionWidth->setValue(currentWidth);
    QHBoxLayout *layoutResolutionWidth = new QHBoxLayout();
    layoutResolutionWidth->addWidget(new QLabel("Width:"));
    layoutResolutionWidth->addWidget(m_resolutionWidth);
    QWidget *widgetResolutionWidth = new QWidget();
    widgetResolutionWidth->setLayout(layoutResolutionWidth);

    m_resolutionHeight = new QSpinBox();
    m_resolutionHeight->setMinimum(0);
    m_resolutionHeight->setMaximum(16384);
    m_resolutionHeight->setValue(currentHeight);
    QHBoxLayout *layoutResolutionHeight = new QHBoxLayout();
    layoutResolutionHeight->addWidget(new QLabel("Height:"));
    layoutResolutionHeight->addWidget(m_resolutionHeight);
    QWidget *widgetResolutionHeight = new QWidget();
    widgetResolutionHeight->setLayout(layoutResolutionHeight);

    QGroupBox *resolutionGroupBox = new QGroupBox("Resolution:");
    QHBoxLayout *layoutResolution = new QHBoxLayout();
    layoutResolution->addWidget(widgetResolutionWidth);
    layoutResolution->addWidget(widgetResolutionHeight);
    layoutResolution->setAlignment(Qt::AlignLeft);
    resolutionGroupBox->setLayout(layoutResolution);

    /* Render parameters widget */
    m_samples = new QSpinBox();
    m_samples->setMinimum(0);
    m_samples->setMaximum(65536);
    m_samples->setValue(renderInfo.samples);
    m_samples->setFixedWidth(70);
    QHBoxLayout *layoutSamples = new QHBoxLayout();
    layoutSamples->addWidget(new QLabel("Samples:"));
    layoutSamples->addWidget(m_samples);
    QWidget *widgetSamples = new QWidget();
    widgetSamples->setLayout(layoutSamples);
    widgetSamples->setFixedWidth(200);

    m_depth = new QSpinBox();
    m_depth->setMinimum(0);
    m_depth->setMaximum(65536);
    m_depth->setValue(renderInfo.depth);
    m_depth->setFixedWidth(70);
    QHBoxLayout *layoutDepth = new QHBoxLayout();
    layoutDepth->addWidget(new QLabel("Max depth:"));
    layoutDepth->addWidget(m_depth);
    QWidget *widgetDepth = new QWidget();
    widgetDepth->setLayout(layoutDepth);
    widgetDepth->setFixedWidth(200);

    m_batchSize = new QSpinBox();
    m_batchSize->setMinimum(1);
    m_batchSize->setMaximum(256);
    m_batchSize->setValue(renderInfo.batchSize);
    m_batchSize->setFixedWidth(70);
    QHBoxLayout *layoutBatchSize = new QHBoxLayout();
    layoutBatchSize->addWidget(new QLabel("Batch size:"));
    layoutBatchSize->addWidget(m_batchSize);
    QWidget *widgetBatchSize = new QWidget();
    widgetBatchSize->setLayout(layoutBatchSize);
    widgetBatchSize->setFixedWidth(200);

    QGroupBox *renderGroupBox = new QGroupBox("Parameters");
    QVBoxLayout *layoutRender = new QVBoxLayout();
    layoutRender->addWidget(widgetSamples);
    layoutRender->addWidget(widgetBatchSize);
    layoutRender->addWidget(widgetDepth);
    layoutRender->setSpacing(0);
    renderGroupBox->setLayout(layoutRender);

    /* Denoise widget */
    m_denoise = new QCheckBox("Denoise");
    m_denoise->setChecked(renderInfo.denoise);

    m_writeIntermediateResults = new QCheckBox("Write intermediate results");
    m_writeIntermediateResults->setChecked(renderInfo.writeAllFiles);

    QGroupBox *denoiseGroupBox = new QGroupBox("Denoise");
    QVBoxLayout *layoutDenoise = new QVBoxLayout();
    layoutDenoise->addWidget(m_denoise);
    layoutDenoise->addWidget(m_writeIntermediateResults);
    layoutDenoise->setSpacing(0);
    denoiseGroupBox->setLayout(layoutDenoise);

    /* Buttons widget */
    m_buttonRender = new QPushButton(tr("Render"));
    connect(m_buttonRender, &QPushButton::released, this, &DialogSceneRender::onButtonRender);
    m_buttonCancel = new QPushButton(tr("Cancel"));
    connect(m_buttonCancel, &QPushButton::released, this, &DialogSceneRender::onButtonCancel);
    QHBoxLayout *layoutButtons = new QHBoxLayout();
    layoutButtons->addWidget(m_buttonCancel);
    layoutButtons->addWidget(m_buttonRender);
    QWidget *widgetButtons = new QWidget();
    widgetButtons->setLayout(layoutButtons);

    /* Main widget */
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widgetRenderOutputFile);
    layoutMain->addWidget(resolutionGroupBox);
    layoutMain->addWidget(renderGroupBox);
    layoutMain->addWidget(denoiseGroupBox);
    layoutMain->addWidget(widgetButtons);
    layoutMain->setSpacing(10);

    setLayout(layoutMain);

    this->setFixedWidth(300);
    this->setFixedHeight(480);
}

RendererPathTracing::RenderInfo DialogSceneRender::getRenderInfo() const
{
    RendererPathTracing::RenderInfo renderInfo;
    renderInfo.samples = m_samples->value();
    renderInfo.depth = m_depth->value();
    renderInfo.batchSize = m_batchSize->value();
    renderInfo.filename = m_renderOutputFileWidget->text().toStdString();
    renderInfo.fileType = fileTypesFromExtensions.find(m_renderOutputFileTypeWidget->currentText().toStdString())->second;
    renderInfo.width = m_resolutionWidth->value();
    renderInfo.height = m_resolutionHeight->value();
    renderInfo.denoise = m_denoise->isChecked();
    renderInfo.writeAllFiles = m_writeIntermediateResults->isChecked();
    return renderInfo;
}

bool DialogSceneRender::renderRequested() const
{
    return m_renderRequested;
}

void DialogSceneRender::onButtonSetRenderOutputFile()
{
    QString filename = QFileDialog::getSaveFileName(this, "Choose fle", "./");
    if (filename == "")
        return;

    m_renderOutputFileWidget->setText(filename);
}

void DialogSceneRender::onButtonRender()
{
    m_renderRequested = true;

    close();
}

void DialogSceneRender::onButtonCancel()
{
    close();
}
