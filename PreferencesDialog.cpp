#include <QtGui>

#include "PreferencesDialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent)
{
    contentsWidget = new QListWidget;
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    contentsWidget->setMinimumWidth(128);
    contentsWidget->setMaximumWidth(128);
    contentsWidget->setCurrentRow(0);

    QListWidgetItem* generalItem = new QListWidgetItem(contentsWidget);
    generalItem->setText(tr("General"));
    generalItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* modelViewItem = new QListWidgetItem(contentsWidget);
    modelViewItem->setText(tr("Model View"));
    modelViewItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* gridItem = new QListWidgetItem(contentsWidget);
    gridItem->setText(tr("  Grid"));
    gridItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* lightingItem = new QListWidgetItem(contentsWidget);
    lightingItem->setText(tr("  Lighting"));
    lightingItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* guidesItem = new QListWidgetItem(contentsWidget);
    guidesItem->setText(tr("  Image Guides"));
    guidesItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* keysItem = new QListWidgetItem(contentsWidget);
    keysItem->setText(tr("Custom Keys"));
    keysItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    contentsWidget->setCurrentItem(generalItem);

    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(changePage(QListWidgetItem*,QListWidgetItem*)));


    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(new GeneralPage);
    pagesWidget->addWidget(new ModelViewPage);
    pagesWidget->addWidget(new GridPage);
    pagesWidget->addWidget(new LightingPage);
    pagesWidget->addWidget(new GuidesPage);
    pagesWidget->addWidget(new WIPPage);

    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    QPushButton *okButton = new QPushButton(tr("OK"));
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(okButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Sproxel Settings"));
    resize(QSize(537, 305));
}

void PreferencesDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}


GeneralPage::GeneralPage(QWidget *parent)
    : QWidget(parent)
{
    QCheckBox* saveWindowPositions = new QCheckBox("Save Window Positions", this);
    QCheckBox* frameOnOpen = new QCheckBox("Frame Model On Open", this);

    QVBoxLayout *stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(saveWindowPositions);
    stuffzLayout->addWidget(frameOnOpen);

    QGroupBox *configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

ModelViewPage::ModelViewPage(QWidget *parent)
    : QWidget(parent)
{
    QLabel* backgroundColor = new QLabel("Window Background Color", this);
    QLabel* voxelDisplay = new QLabel("Voxel Display Style", this);

    QVBoxLayout *stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(backgroundColor);
    stuffzLayout->addWidget(voxelDisplay);

    QGroupBox *configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


GridPage::GridPage(QWidget *parent)
    : QWidget(parent)
{
    QLabel* gridColor = new QLabel("Grid Color", this);
    QLabel* gridSize = new QLabel("Grid Size", this);
    QLabel* gridCellSize = new QLabel("Grid Cell Size", this);

    QVBoxLayout *stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(gridColor);
    stuffzLayout->addWidget(gridSize);
    stuffzLayout->addWidget(gridCellSize);

    QGroupBox *configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


LightingPage::LightingPage(QWidget *parent)
    : QWidget(parent)
{
    QLabel* fixedDir = new QLabel("Fix Light Direction", this);
    QLabel* lightDir = new QLabel("Light Direction", this);
    QLabel* lightColor = new QLabel("Light Color", this);
    QLabel* ambientColor = new QLabel("Ambient Color", this);

    QVBoxLayout *stuffzLayout = new QVBoxLayout;
    stuffzLayout->addWidget(fixedDir);
    stuffzLayout->addWidget(lightDir);
    stuffzLayout->addWidget(lightColor);
    stuffzLayout->addWidget(ambientColor);

    QGroupBox *configGroup = new QGroupBox();
    configGroup->setLayout(stuffzLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


GuidesPage::GuidesPage(QWidget *parent)
    : QWidget(parent)
{
    QGroupBox *packagesGroup = new QGroupBox(tr("Guide Filenames"));

    QLabel *xyLabel = new QLabel(tr("XY Plane:"));
    QLineEdit *xyEdit = new QLineEdit;

    QLabel *xzLabel = new QLabel(tr("XZ Plane:"));
    QLineEdit *xzEdit = new QLineEdit;

    QLabel *yzLabel = new QLabel(tr("YZ Plane:"));
    QLineEdit *yzEdit = new QLineEdit;

    QGridLayout *packagesLayout = new QGridLayout;
    packagesLayout->addWidget(xyLabel, 0, 0);
    packagesLayout->addWidget(xyEdit, 0, 1);
    packagesLayout->addWidget(xzLabel, 1, 0);
    packagesLayout->addWidget(xzEdit, 1, 1);
    packagesLayout->addWidget(yzLabel, 2, 0);
    packagesLayout->addWidget(yzEdit, 2, 1);
    packagesGroup->setLayout(packagesLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(packagesGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

WIPPage::WIPPage(QWidget *parent)
    : QWidget(parent)
{
    QLabel* wip = new QLabel("Soon", this);
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(wip);
    setLayout(mainLayout);
}
