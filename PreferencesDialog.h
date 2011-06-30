#ifndef _SETTINGS_DIALOG_H_
#define _SETTINGS_DIALOG_H_

#include <QDialog>
#include <QSettings>
#include <QListWidget>
#include <QStackedWidget>

class GeneralPage;
class ModelViewPage;
class GridPage;
class LightingPage;
class GuidesPage;
class PalettePage;
class WIPPage;

// Main preferences dialog
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget* parent = 0, QSettings* appSettings = NULL);

public slots:
    void changePage(QListWidgetItem* current, QListWidgetItem* previous);
    void reject();

signals:
    void preferenceChanged();

private:
    QSettings* m_pAppSettings;
    QListWidget* m_pContentsWidget;
    QStackedWidget* m_pPagesWidget;
    
    // Keep track of all the sub-pages
    GeneralPage* m_pGeneralPage;
    ModelViewPage* m_pModelViewPage;
    GridPage* m_pGridPage;
    LightingPage* m_pLightingPage;
    GuidesPage* m_pGuidesPage;
    PalettePage* m_pPalettePage;
    WIPPage* m_pWIPPage;
};


// Preference sub-pages
class GeneralPage : public QWidget
{   Q_OBJECT
public:
    GeneralPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    bool m_saveWindowPositionsOrig;
    bool m_frameOnOpenOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setSaveWindowPositions(int state);
    void setFrameOnOpen(int state);
};


class ModelViewPage : public QWidget
{   Q_OBJECT
public:
    ModelViewPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    QColor m_backgroundColorOrig;
    int m_voxelDisplayOrig;
    void restoreOriginals();
};


class GridPage : public QWidget
{   Q_OBJECT
public:
    GridPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    QColor m_gridColorOrig;
    int m_gridSizeOrig;
    int m_gridCellSizeOrig;
    void restoreOriginals();

signals:
    void preferenceChanged();

public slots:
    void setGridSize(int value);
    void setgridCellSize(int value);
};


class LightingPage : public QWidget
{   Q_OBJECT
public:
    LightingPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class GuidesPage : public QWidget
{   Q_OBJECT
public:
    GuidesPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class PalettePage : public QWidget
{   Q_OBJECT
public:
    PalettePage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    void restoreOriginals();
};


class WIPPage : public QWidget
{   Q_OBJECT
public:
    WIPPage(QWidget* parent = NULL, QSettings* appSettings = NULL);
    
    QSettings* m_pAppSettings;
    void restoreOriginals();
};

#endif
