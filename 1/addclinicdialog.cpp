#include "addclinicdialog.h"
#include "ui_addclinicdialog.h"
#include <QMessageBox>
#include <QRegularExpression>

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

    // 联系电话非空时校验格式
    QString clinicPhone = ui->editPhone->text().trimmed();
    if (!clinicPhone.isEmpty()) {
        QRegularExpression phoneRe("^1[3-9]\\d{9}$");
        if (!phoneRe.match(clinicPhone).hasMatch()) {
            QMessageBox::warning(this, "提示", "联系电话格式不正确，请输入有效的11位手机号码！");
            return;
        }
    }

    resultClinic.clinicNo     = ui->editClinicNo->text().trimmed();
    resultClinic.clinicName   = ui->editClinicName->text().trimmed();
    resultClinic.doctorNo     = ui->editDoctorNo->text().trimmed();
    resultClinic.consultTime  = ui->editConsultTime->text().trimmed();
    resultClinic.capacity     = ui->spinCapacity->value();
    resultClinic.currentCount = 0;
    resultClinic.address      = ui->editAddress->text().trimmed();
    resultClinic.phone        = clinicPhone;

    this->accept();
}

void AddClinicDialog::on_btnCancel_clicked() {
    this->reject();
}

Clinic AddClinicDialog::getClinic() const {
    return resultClinic;
}
