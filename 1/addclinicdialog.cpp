#include "addclinicdialog.h"
#include "ui_addclinicdialog.h"
#include <QMessageBox>

AddClinicDialog::AddClinicDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddClinicDialog)
{
    ui->setupUi(this);
    // 所有界面属性已在 .ui 中设置
}

AddClinicDialog::~AddClinicDialog() {
    delete ui;
}

void AddClinicDialog::on_btnOk_clicked() {
    if (ui->editClinicNo->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "门诊号不能为空！");
        return;
    }
    if (ui->editClinicName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "门诊名称不能为空！");
        return;
    }

    resultClinic.clinicNo     = ui->editClinicNo->text().trimmed();
    resultClinic.clinicName   = ui->editClinicName->text().trimmed();
    resultClinic.doctorNo     = ui->editDoctorNo->text().trimmed();
    resultClinic.consultTime  = ui->editConsultTime->text().trimmed();
    resultClinic.capacity     = ui->spinCapacity->value();
    resultClinic.currentCount = 0;
    resultClinic.address      = ui->editAddress->text().trimmed();
    resultClinic.phone        = ui->editPhone->text().trimmed();

    this->accept();
}

void AddClinicDialog::on_btnCancel_clicked() {
    this->reject();
}

Clinic AddClinicDialog::getClinic() const {
    return resultClinic;
}