#include <QtGui>

#include <iostream>

#include "GLCamera.h"
#include "MainWindow.h"
#include "GLModelWidget.h"

#include <QFileDialog>
#include <QColorDialog>

MainWindow::MainWindow()
{
    m_glModelWidget = new GLModelWidget;

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_glModelWidget);
    setLayout(mainLayout);

    setWindowTitle(tr("Sproxel"));
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;

    if (event->key() == Qt::Key_F)
    {
        m_glModelWidget->frame();
    }
    else if (event->key() >= Qt::Key_Left && event->key() <= Qt::Key_PageDown)
    {
        m_glModelWidget->handleArrows(event);
    }
    else if (ctrlDown && event->key() == Qt::Key_G)
    {
        m_glModelWidget->setDrawGrid(!m_glModelWidget->drawGrid());
    }
    else if (event->key() == Qt::Key_G)
    {
        m_glModelWidget->setDrawVoxelGrid(!m_glModelWidget->drawVoxelGrid());
    }
    else if (event->key() == Qt::Key_Space)
    {
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(), 
                                       m_glModelWidget->activeColor());
        m_glModelWidget->updateGL();
    }
    else if (event->key() == Qt::Key_Delete)
    {
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(), 
                                       Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
        m_glModelWidget->updateGL();
    }
    else if (ctrlDown && event->key() == Qt::Key_C)
    {
        // TODO: Why can i not add the additional two parameters to this?
        QColor color = QColorDialog::getColor(Qt::white, this);
        m_glModelWidget->setActiveColor(Imath::Color4f((float)color.red()/255.0f,
                                                       (float)color.green()/255.0f, 
                                                       (float)color.blue()/255.0f, 
                                                       (float)color.alpha()/255.0f));
    }
    else if (ctrlDown && event->key() == Qt::Key_S)
    {
        QString filename = QFileDialog::getSaveFileName(this, 
            tr("Save fur project (CSV format) file as..."), 
            QString(""),
            tr("CSV Files (*.csv)"));
        if (!filename.isEmpty())
        {
            m_glModelWidget->saveGridCSV(filename.toStdString());
        }
    }
    else if (ctrlDown && event->key() == Qt::Key_O)
    {
        QString filename = QFileDialog::getOpenFileName(this, 
            tr("Select CSV file..."), 
            QString(),
            tr("CSV Files (*.csv)"));
        if (!filename.isEmpty())
        {
            m_glModelWidget->loadGridCSV(filename.toStdString());
        }
    }
}

// void MainWindow::keyReleaseEvent(QKeyEvent *event)
// {
// }

// // Steal keypresses from your children!
// bool MainWindow::eventFilter(QObject* qo, QEvent* ev)
// {
//     if (ev->type() != QEvent::KeyPress) return false;
//     keyPressEvent(dynamic_cast<QKeyEvent*>(ev));
//     return true;
// }

