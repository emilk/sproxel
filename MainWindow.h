#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QMenu>
#include <QWidget>
#include <QAction>
#include <QToolBar>
#include <QMainWindow>

#include "NewGridDialog.h"
#include "PaletteWidget.h"

#define SPROXEL_VERSION "0.2"
#define BASE_WINDOW_TITLE (tr("Sproxel " SPROXEL_VERSION))

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
    PaletteWidget* m_paletteWidget;
    
    // Actions
    QAction* m_actQuit;

    QAction* m_actUndo;
    QAction* m_actRedo;
    QAction* m_actShiftUp;
    QAction* m_actShiftDown;
    QAction* m_actShiftWrap;
    QAction* m_actUpRes;
    QAction* m_actDownRes;

    QAction* m_actViewGrid;
    QAction* m_actViewVoxgrid;
    QAction* m_actViewBBox;

    QAction* m_actFileNew;
    QAction* m_actFileOpen;
    QAction* m_actFileSave;
    QAction* m_actFileSaveAs;

    QActionGroup* m_toolbarActionGroup;
    QAction* m_actToolSplat;
    QAction* m_actToolReplace;
    QAction* m_actToolDropper;
    QAction* m_actToolFlood;
    QAction* m_actToolEraser;
    QAction* m_actToolSlab;
    QAction* m_actToolRay;

    // Locals
    std::string m_activeFilename;

public slots:
    void newGrid();

    void saveFile();
    void saveFileAs();
    void openFile();

    void shiftUp();
    void shiftDown();

    void upRes();
    void downRes();

    void setToolSplat(bool stat);
    void setToolFlood(bool stat);
    void setToolRay(bool stat);
    void setToolDropper(bool stat);
    void setToolEraser(bool stat);
    void setToolReplace(bool stat);
    void setToolSlab(bool stat);

    void reactToModified(bool value);
};

#endif
