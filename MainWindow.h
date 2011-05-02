#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QToolBar>

#include "NewGridDialog.h"

class QSlider;
class GLModelWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow( QWidget *parent=NULL );

protected:
    void keyPressEvent(QKeyEvent* event);
    
private:
    GLModelWidget* m_glModelWidget;

    // Menus
    QMenu* m_menuFile;
    QMenu* m_menuEdit;
    QMenu* m_menuView;
    QMenu* m_menuWindow;
    
    // Docking windows and toolbars
    QToolBar* m_toolbar;
    QDockWidget* m_paletteDocker;
    
    // Actions
    QAction* m_actQuit;

    QAction* m_actUndo;
    QAction* m_actRedo;

    QAction* m_actViewGrid;
    QAction* m_actViewVoxgrid;
    QAction* m_actViewBBox;

    QAction* m_actFileNew;
    QAction* m_actFileOpen;
    QAction* m_actFileSave;

public slots:
    void newGrid();

    void saveFile();
    void openFile();

    //bool eventFilter(QObject* qo, QEvent* ev);
};

#endif
