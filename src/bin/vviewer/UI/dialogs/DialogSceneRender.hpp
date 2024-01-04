#ifndef __DialogSceneRender_hpp__
#define __DialogSceneRender_hpp__

#include <cstdint>
#include <qcombobox.h>
#include <qdialog.h>
#include <qlistwidget.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcheckbox.h>

#include "vengine/core/io/FileTypes.hpp"
#include "vengine/core/Renderer.hpp"

using namespace vengine;

/* A dialog to render a scene */
class DialogSceneRender : public QDialog
{
    Q_OBJECT
public:
    DialogSceneRender(QWidget *parent, const RendererPathTracing::RenderInfo &renderInfo);

    RendererPathTracing::RenderInfo getRenderInfo() const;

    bool renderRequested() const;

private:
    bool m_renderRequested = false;

    QLineEdit *m_renderOutputFileWidget = nullptr;
    QPushButton *m_renderOutputFileChangeButton = nullptr;
    std::string m_renderOutputFileName = "";

    QComboBox *m_renderOutputFileTypeWidget = nullptr;

    QSpinBox *m_resolutionWidth = nullptr;
    QSpinBox *m_resolutionHeight = nullptr;

    QSpinBox *m_samples = nullptr;
    QSpinBox *m_depth = nullptr;
    QSpinBox *m_batchSize = nullptr;

    QCheckBox *m_denoise = nullptr;
    QCheckBox *m_writeIntermediateResults = nullptr;

    QPushButton *m_buttonRender = nullptr;
    QPushButton *m_buttonCancel = nullptr;

private Q_SLOTS:
    void onButtonSetRenderOutputFile();
    void onButtonRender();
    void onButtonCancel();
};

#endif