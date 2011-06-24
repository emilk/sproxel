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

    QListWidgetItem* paletteItem = new QListWidgetItem(contentsWidget);
    paletteItem->setText(tr("Palette"));
    paletteItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem* keysItem = new QListWidgetItem(contentsWidget);
    keysItem->setText(tr("Custom Keys"));
    keysItem->setFlags(Qt::ItemIsSelectable);

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
    pagesWidget->addWidget(new PalettePage);
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
    QSpinBox *sizeSpinBox = new QSpinBox;
    sizeSpinBox->setRange(0, 200);
    sizeSpinBox->setSingleStep(1);
    sizeSpinBox->setValue(16);

    QLabel* gridCellSize = new QLabel("Grid Cell Size", this);
    QSpinBox *cellSizeSpinBox = new QSpinBox;
    cellSizeSpinBox->setRange(1, 200);
    cellSizeSpinBox->setSingleStep(1);
    cellSizeSpinBox->setValue(1);

    QGroupBox* gridGroup = new QGroupBox(tr("Guide Filenames"));

    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->addWidget(gridColor, 0, 0);
    gridLayout->addWidget(gridSize, 1, 0);
    gridLayout->addWidget(sizeSpinBox, 1, 1);
    gridLayout->addWidget(gridCellSize, 2, 0);
    gridLayout->addWidget(cellSizeSpinBox, 2, 1);
    gridGroup->setLayout(gridLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(gridGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


LightingPage::LightingPage(QWidget *parent)
    : QWidget(parent)
{
    QCheckBox* fixedDir = new QCheckBox("Fixed Light Direction", this);
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
    QGroupBox *guidesGroup = new QGroupBox(tr("Guide Filenames"));

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
    guidesGroup->setLayout(packagesLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(guidesGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


PalettePage::PalettePage(QWidget *parent)
    : QWidget(parent)
{
    QGroupBox *guidesGroup = new QGroupBox(tr("Guide Filenames"));

    QCheckBox* saveActiveColors = new QCheckBox("Save Active Colors", this);

    QLabel *filenameLabel = new QLabel(tr("Palette filename:"));
    QLineEdit *filenameEdit = new QLineEdit;

    QGridLayout *packagesLayout = new QGridLayout;
    packagesLayout->addWidget(saveActiveColors, 0, 0, 1, 2);
    packagesLayout->addWidget(filenameLabel, 1, 0);
    packagesLayout->addWidget(filenameEdit, 1, 1);
    guidesGroup->setLayout(packagesLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(guidesGroup);
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
