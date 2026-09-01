#include "bookdialog.h"
#include "ui_bookdialog.h"
#include <QMessageBox>
#include <QRegularExpression>

BookDialog::BookDialog(QList<Clinic> &clinics, QWidget *parent)
    : QDialog(parent), ui(new Ui::BookDialog),
    clinicList(clinics), selectedIndex(-1)
{
    ui->setupUi(this);

    // 只有下拉框的选项需要代码动态填充（因为要过滤已满的门诊）
    ui->comboClinic->clear();
    for (int i = 0; i < clinicList.size(); i++) {
        if (clinicList[i].canBook()) {
            QString displayText = clinicList[i].clinicName
                                  + "（剩余" + QString::number(clinicList[i].getAvailableSlots()) + "号）";
            ui->comboClinic->addItem(displayText, i);
        }
    }
    if (ui->comboClinic->count() == 0) {
        ui->comboClinic->addItem("（无可预约门诊）");
    }
}

BookDialog::~BookDialog() {
    delete ui;
}

void BookDialog::on_btnOk_clicked() {
    // 先检查门诊选择：占位项无 data，说明没有可预约门诊
    QVariant clinicData = ui->comboClinic->currentData();
    if (!clinicData.isValid()) {
        QMessageBox::warning(this, "提示", "没有可预约的门诊！");
        return;
    }
    selectedIndex = clinicData.toInt();
    if (selectedIndex < 0 || selectedIndex >= clinicList.size()) {
        QMessageBox::warning(this, "提示", "请选择有效的门诊！");
        return;
    }
    if (!clinicList[selectedIndex].canBook()) {
        QMessageBox::warning(this, "预约失败", "该门诊号源已满！");
        return;
    }

    if (ui->editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "姓名不能为空！");
        return;
    }
    QString phone = ui->editPhone->text().trimmed();
    if (phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "电话不能为空！");
        return;
    }
    // 电话格式校验：中国大陆手机号 1[3-9]xxxxxxxxx
    QRegularExpression phoneRe("^1[3-9]\\d{9}$");
    if (!phoneRe.match(phone).hasMatch()) {
        QMessageBox::warning(this, "提示", "请输入有效的11位手机号码！");
        return;
    }
    if (ui->editAppointTime->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "预约时间不能为空！");
        return;
    }

    resultAppoint.name        = ui->editName->text().trimmed();
    resultAppoint.phone       = phone;
    resultAppoint.gender      = ui->comboGender->currentText();
    resultAppoint.age         = ui->spinAge->value();
    resultAppoint.appointTime = ui->editAppointTime->text().trimmed();

    this->accept();
}

void BookDialog::on_btnCancel_clicked() {
    this->reject();
}

int BookDialog::getSelectedClinicIndex() const {
    return selectedIndex;
}

Appointment BookDialog::getAppointment() const {
    return resultAppoint;
}
