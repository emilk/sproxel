#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QMenu>
#include <QWidget>
#include <QAction>
#include <QToolBar>
#include <QSettings>
#include <QMainWindow>

#include "NewGridDialog.h"
#include "PaletteWidget.h"

#define SPROXEL_VERSION "0.3"
#define BASE_WINDOW_TITLE (tr("Sproxel " SPROXEL_VERSION))

class GLModelWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString& initialFilename, QWidget* parent=NULL);
    QSettings& appSettings() { return m_appSettings; }

protected:
    void closeEvent(QCloseEvent* event);
    void keyPressEvent(QKeyEvent* event);

private:
    QSettings m_appSettings;

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
    QAction* m_actMirror;
    QAction* m_actUpRes;
    QAction* m_actDownRes;
    QAction* m_actPreferences;

    QAction* m_actViewGrid;
    QAction* m_actViewVoxgrid;
    QAction* m_actViewBBox;

    QAction* m_actFileNew;
    QAction* m_actFileOpen;
    QAction* m_actFileSave;
    QAction* m_actFileSaveAs;

    QAction* m_actFileImport;
    QAction* m_actFileExportGrid;

    QActionGroup* m_toolbarActionGroup;
    QAction* m_actToolSplat;
    QAction* m_actToolReplace;
    QAction* m_actToolDropper;
    QAction* m_actToolFlood;
    QAction* m_actToolEraser;
    QAction* m_actToolSlab;
    QAction* m_actToolRay;

    // Locals
    QString m_activeFilename;

    // Functions
    int fileModifiedDialog();

public slots:
    void newGrid();

    void saveFile();
    void saveFileAs();
    void openFile();

    void import();
    void exportGrid();

    void shiftUp();
    void shiftDown();
    void mirror();

    void upRes();
    void downRes();

    void editPreferences();

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
