#ifndef NEWGRIDDIALOG_H
#define NEWGRIDDIALOG_H

#include <QDialog>

#include <ImathVec.h>

namespace Ui {
    class NewGridDialog;
}

class NewGridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewGridDialog(QWidget *parent = 0);
    ~NewGridDialog();

    Imath::V3i getVoxelSize();
    bool isIndexed();

private:
    Ui::NewGridDialog *ui;
};

#endif // NEWGRIDDIALOG_H
