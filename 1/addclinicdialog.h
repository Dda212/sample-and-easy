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

    // 切换到"编辑"模式：预填已有门诊数据，并锁定门诊号（门诊号是主键）
    void setEditClinic(const Clinic &clinic);

    Clinic getClinic() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::AddClinicDialog *ui;
    Clinic resultClinic;
};

#endif