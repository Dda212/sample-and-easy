#include "addclinicdialog.h"
#include "ui_addclinicdialog.h"
#include <QMessageBox>
#include <QRegularExpression>

AddClinicDialog::AddClinicDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddClinicDialog)
{
    ui->setupUi(this);
}

AddClinicDialog::~AddClinicDialog() {
    delete ui;
}

void AddClinicDialog::setEditClinic(const Clinic &clinic) {
    setWindowTitle("编辑门诊");
    ui->editClinicNo->setText(clinic.clinicNo);
    ui->editClinicNo->setReadOnly(true);
    ui->editClinicName->setText(clinic.clinicName);
    ui->editDoctorNo->setText(clinic.doctorNo);
    ui->editConsultTime->setText(clinic.consultTime);
    ui->spinCapacity->setValue(clinic.capacity);
    ui->editAddress->setText(clinic.address);
    ui->editPhone->setText(clinic.phone);
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
    // currentCount 由 Clinic 构造函数初始化为 0，封装后不允许外部直接赋值
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
