#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);

}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}

Imath::V3i NewProjectDialog::getVoxelSize()
 {
    return Imath::V3i( ui->width->value(),
                        ui->height->value(),
                        ui->depth->value() );
 }
