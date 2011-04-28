#include <QtGui>

#include <iostream>

#include "GLCamera.h"
#include "MainWindow.h"
#include "GLModelWidget.h"

#include <QFileDialog>
#include <QColorDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_glModelWidget = new GLModelWidget;

    // Actions & Menus
    menuBar()->show();
    m_menuFile = menuBar()->addMenu("&File");
    m_toolbarFile = addToolBar("&File");

    m_actFileNew = new QAction("&New", this);
    m_actFileNew->setShortcut(Qt::CTRL + Qt::Key_N);
    m_menuFile->addAction(m_actFileNew);
    connect(m_actFileNew, SIGNAL(triggered()),
            this, SLOT(newGrid()));

    m_menuFile->addSeparator();

    m_actFileOpen = new QAction("&Open", this);
    m_actFileOpen->setShortcut(Qt::CTRL + Qt::Key_O);
    m_menuFile->addAction(m_actFileOpen);
    connect(m_actFileOpen, SIGNAL(triggered()), 
            this, SLOT(openFile()));

    m_actFileSave = new QAction("&Save", this);
    m_actFileSave->setShortcut(Qt::CTRL + Qt::Key_S);
    m_menuFile->addAction(m_actFileSave);
    connect(m_actFileSave, SIGNAL(triggered()), 
            this, SLOT(saveFile()));

    m_menuFile->addSeparator();
    m_actQuit = new QAction("&Quit", this);
    m_menuFile->addAction(m_actQuit);
    m_toolbarFile->addAction(m_actQuit);
    connect(m_actQuit, SIGNAL(triggered()), 
            qApp, SLOT(quit()));


    // ------ edit menu
    m_menuEdit = menuBar()->addMenu("&Edit");

    m_actUndo = new QAction("Undo", this);
    m_actUndo->setShortcut(Qt::CTRL + Qt::Key_Z);
    m_menuEdit->addAction(m_actUndo);
    connect(m_actUndo, SIGNAL(triggered()),
            m_glModelWidget, SLOT(undo()));

    m_actRedo = new QAction("Redo", this);
    m_actRedo->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
    m_menuEdit->addAction(m_actRedo);
    connect(m_actRedo, SIGNAL(triggered()),
            m_glModelWidget, SLOT(redo()));


    // ------ view menu
    m_menuView = menuBar()->addMenu("&View");

    m_actViewGrid = new QAction("View Grid", this);
    m_actViewGrid->setShortcut(Qt::CTRL + Qt::Key_G);
    m_actViewGrid->setCheckable(true);
    m_actViewGrid->setChecked(m_glModelWidget->drawGrid());
    m_menuView->addAction(m_actViewGrid);
    connect(m_actViewGrid, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawGrid(bool)));

    m_actViewVoxgrid = new QAction("Voxel Grid", this);
    m_actViewVoxgrid->setShortcut(Qt::Key_G);
    m_actViewVoxgrid->setCheckable(true);
    m_actViewVoxgrid->setChecked(m_glModelWidget->drawVoxelGrid());
    m_menuView->addAction(m_actViewVoxgrid);
    connect(m_actViewVoxgrid, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawVoxelGrid(bool)));

    m_actViewBBox = new QAction("Bounding Box", this);
    m_actViewBBox->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actViewBBox->setCheckable(true);
    m_actViewBBox->setChecked(m_glModelWidget->drawBoundingBox());
    m_menuView->addAction(m_actViewBBox);
    connect(m_actViewBBox, SIGNAL(toggled(bool)),
            m_glModelWidget, SLOT(setDrawBoundingBox(bool)));

    // add the voxel widget
    //QVBoxLayout *mainLayout = new QVBoxLayout;
    //mainLayout->addWidget(m_glModelWidget);
    //setLayout(mainLayout);
    setCentralWidget(m_glModelWidget);

    setWindowTitle(tr("Sproxel"));
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    //const bool altDown = event->modifiers() & Qt::AltModifier;
    const bool ctrlDown = event->modifiers() & Qt::ControlModifier;

    if (event->key() == Qt::Key_F)
    {
        m_glModelWidget->frame();
    }
    else if (event->key() >= Qt::Key_Left && event->key() <= Qt::Key_PageDown)
    {
        m_glModelWidget->handleArrows(event);
    }
    else if (event->key() == Qt::Key_Space)
    {
        // It's okay to call setVoxelColor once on the model widget, but any more requires an internal wrapper
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(), 
                                       m_glModelWidget->activeColor());
        m_glModelWidget->updateGL();
    }
    else if (event->key() == Qt::Key_Delete)
    {
        // It's okay to call setVoxelColor once on the model widget, but any more requires an internal wrapper
        m_glModelWidget->setVoxelColor(m_glModelWidget->activeVoxel(), 
                                       Imath::Color4f(0.0f, 0.0f, 0.0f, 0.0f));
        m_glModelWidget->updateGL();
    }
    else if (event->key() == Qt::Key_X)
    {
        m_glModelWidget->setCurrentAxis(GLModelWidget::X_AXIS);
    }
    else if (event->key() == Qt::Key_Y)
    {
        m_glModelWidget->setCurrentAxis(GLModelWidget::Y_AXIS);
    }
    else if (event->key() == Qt::Key_Z)
    {
        m_glModelWidget->setCurrentAxis(GLModelWidget::Z_AXIS);
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

}

void MainWindow::saveFile()
{
    QString filename = QFileDialog::getSaveFileName(this,
        tr("Save voxels (CSV format) file as..."),
        QString(""),
        tr("CSV Files (*.csv)"));
    if (!filename.isEmpty())
    {
        m_glModelWidget->saveGridCSV(filename.toStdString());
    }
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Select CSV file to Open..."),
        QString(),
        tr("CSV Files (*.csv)"));
    if (!filename.isEmpty())
    {
        m_glModelWidget->loadGridCSV(filename.toStdString());
    }
}

void MainWindow::newGrid()
{
    NewGridDialog dlg(this);

    dlg.setModal(true);
    if (dlg.exec())
    {
        m_glModelWidget->resizeVoxelGrid(dlg.getVoxelSize());
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

