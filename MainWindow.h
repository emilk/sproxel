#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QToolBar>

#include "newprojectdialog.h"

class QSlider;
class GLModelWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow( QWidget *parent=NULL );

protected:
    void keyPressEvent(QKeyEvent* event);
    //void keyReleaseEvent(QKeyEvent *event);
    
private:
    GLModelWidget* m_glModelWidget;

    // Menus & Toolbars
    QMenu *m_menuFile, *m_menuView;
    QToolBar *m_toolbarFile;
    
    // Actions
    QAction *m_actQuit;

    QAction *m_actViewGrid;
    QAction *m_actViewVoxgrid;
    QAction *m_actViewBBox;

    QAction *m_actFileNew;
    QAction *m_actFileOpen;
    QAction *m_actFileSave;

public slots:
    void newGrid();

    void saveFile();
    void openFile();

    //bool eventFilter(QObject* qo, QEvent* ev);
};

#endif
