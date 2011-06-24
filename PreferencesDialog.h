#ifndef _SETTINGS_DIALOG_H_
#define _SETTINGS_DIALOG_H_

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>

// Main preferences dialog
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QWidget *parent = 0);

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    QListWidget* contentsWidget;
    QStackedWidget* pagesWidget;
};

// Preference sub-pages
class GeneralPage : public QWidget
{
public:
    GeneralPage(QWidget* parent = 0);
};

class ModelViewPage : public QWidget
{
public:
    ModelViewPage(QWidget* parent = 0);
};

class GridPage : public QWidget
{
public:
    GridPage(QWidget* parent = 0);
};

class LightingPage : public QWidget
{
public:
    LightingPage(QWidget* parent = 0);
};

class GuidesPage : public QWidget
{
public:
    GuidesPage(QWidget* parent = 0);
};

class PalettePage : public QWidget
{
public:
    PalettePage(QWidget* parent = 0);
};

class WIPPage : public QWidget
{
public:
    WIPPage(QWidget* parent = 0);
};

#endif
