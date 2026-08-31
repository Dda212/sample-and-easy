#ifndef ADDCLINICDIALOG_H
#define ADDCLINICDIALOG_H

#include <QDialog>
#include "clinic.h"

namespace Ui {
class AddClinicDialog;
}

class AddClinicDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddClinicDialog(QWidget *parent = nullptr);
    ~AddClinicDialog();

    Clinic getClinic() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::AddClinicDialog *ui;
    Clinic resultClinic;
};

#endif