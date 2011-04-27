#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"

NewGridDialog::NewGridDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewGridDialog)
{
    ui->setupUi(this);
}

NewGridDialog::~NewGridDialog()
{
    delete ui;
}

Imath::V3i NewGridDialog::getVoxelSize()
{
    return Imath::V3i(ui->width->value(),
                      ui->height->value(),
                      ui->depth->value());
}
