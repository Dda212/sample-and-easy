#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addclinicdialog.h"
#include "bookdialog.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 所有界面属性已在 .ui 中设置，这里只需加载数据
    loadData();
}

MainWindow::~MainWindow() {
    saveData();
    delete ui;
}

// ========== 查看所有门诊 ==========
void MainWindow::on_btnViewClinics_clicked() {
    refreshClinicTable();
}

// ========== 添加门诊 ==========
void MainWindow::on_btnAddClinic_clicked() {
    AddClinicDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Clinic newClinic = dialog.getClinic();

        // 检查门诊号是否重复
        for (int i = 0; i < clinicList.size(); i++) {
            if (clinicList[i].clinicNo == newClinic.clinicNo) {
                QMessageBox::warning(this, "添加失败",
                                     "门诊号 " + newClinic.clinicNo + " 已存在！");
                return;
            }
        }

        clinicList.append(newClinic);
        refreshClinicTable();
        QMessageBox::information(this, "成功", "门诊添加成功！");
    }
}

// ========== 预约门诊 ==========
void MainWindow::on_btnBook_clicked() {
    if (clinicList.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有门诊信息，请先添加门诊！");
        return;
    }

    BookDialog dialog(clinicList, this);
    if (dialog.exec() == QDialog::Accepted) {
        int idx = dialog.getSelectedClinicIndex();
        Appointment app = dialog.getAppointment();

        bool success = clinicList[idx].addAppointment(app);
        if (success) {
            QMessageBox::information(this, "预约成功",
                                     "已成功预约：" + clinicList[idx].clinicName
                                         + "\n预约人：" + app.name);
            refreshClinicTable();
        } else {
            QMessageBox::warning(this, "预约失败", "该门诊已满，无法预约！");
        }
    }
}

// ========== 查询预约（切换显示/隐藏） ==========
void MainWindow::on_btnQuery_clicked() {
    bool isVisible = ui->editSearchPhone->isVisible();

    if (!isVisible) {
        // 显示查询区域
        ui->editSearchPhone->setVisible(true);
        ui->comboSearchClinic->setVisible(true);
        ui->tableQuery->setVisible(true);

        // 填充门诊下拉框
        ui->comboSearchClinic->clear();
        ui->comboSearchClinic->addItem("-- 按门诊查询 --", -1);
        for (int i = 0; i < clinicList.size(); i++) {
            ui->comboSearchClinic->addItem(clinicList[i].clinicName, i);
        }
    } else {
        // 隐藏查询区域
        ui->editSearchPhone->setVisible(false);
        ui->comboSearchClinic->setVisible(false);
        ui->tableQuery->setVisible(false);
    }
}

// ========== 退出系统 ==========
void MainWindow::on_btnExit_clicked() {
    saveData();
    this->close();
}

// ========== 按门诊下拉框查询 ==========
void MainWindow::on_comboSearchClinic_currentIndexChanged(int index) {
    int clinicIdx = ui->comboSearchClinic->itemData(index).toInt();
    if (clinicIdx >= 0 && clinicIdx < clinicList.size()) {
        refreshQueryByClinic(clinicIdx);
    }
}

// ========== 按电话回车查询 ==========
void MainWindow::on_editSearchPhone_returnPressed() {
    QString phone = ui->editSearchPhone->text().trimmed();
    if (phone.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入电话号码！");
        return;
    }
    refreshQueryByPhone(phone);
}

// ========== 刷新门诊列表表格 ==========
void MainWindow::refreshClinicTable() {
    ui->tableClinics->setRowCount(clinicList.size());
    for (int i = 0; i < clinicList.size(); i++) {
        ui->tableClinics->setItem(i, 0, new QTableWidgetItem(clinicList[i].clinicNo));
        ui->tableClinics->setItem(i, 1, new QTableWidgetItem(clinicList[i].clinicName));
        ui->tableClinics->setItem(i, 2, new QTableWidgetItem(clinicList[i].doctorNo));
        ui->tableClinics->setItem(i, 3, new QTableWidgetItem(clinicList[i].consultTime));
        ui->tableClinics->setItem(i, 4, new QTableWidgetItem(QString::number(clinicList[i].capacity)));
        ui->tableClinics->setItem(i, 5, new QTableWidgetItem(QString::number(clinicList[i].currentCount)));
        ui->tableClinics->setItem(i, 6, new QTableWidgetItem(QString::number(clinicList[i].getAvailableSlots())));
    }
}

// ========== 按门诊刷新查询结果 ==========
void MainWindow::refreshQueryByClinic(int clinicIdx) {
    Clinic &clinic = clinicList[clinicIdx];
    ui->tableQuery->setRowCount(clinic.appointments.size());

    for (int i = 0; i < clinic.appointments.size(); i++) {
        Appointment &app = clinic.appointments[i];
        ui->tableQuery->setItem(i, 0, new QTableWidgetItem(app.name));
        ui->tableQuery->setItem(i, 1, new QTableWidgetItem(app.phone));
        ui->tableQuery->setItem(i, 2, new QTableWidgetItem(app.gender));
        ui->tableQuery->setItem(i, 3, new QTableWidgetItem(QString::number(app.age)));
        ui->tableQuery->setItem(i, 4, new QTableWidgetItem(app.appointTime));
        ui->tableQuery->setItem(i, 5, new QTableWidgetItem(clinic.clinicName));
    }
}

// ========== 按电话刷新查询结果 ==========
void MainWindow::refreshQueryByPhone(const QString &phone) {
    QList<QPair<Appointment, QString>> results;
    for (int i = 0; i < clinicList.size(); i++) {
        for (int j = 0; j < clinicList[i].appointments.size(); j++) {
            if (clinicList[i].appointments[j].phone == phone) {
                results.append(qMakePair(clinicList[i].appointments[j],
                                         clinicList[i].clinicName));
            }
        }
    }

    ui->tableQuery->setRowCount(results.size());
    if (results.isEmpty()) {
        QMessageBox::information(this, "查询结果", "未找到该电话的预约记录。");
        return;
    }

    for (int i = 0; i < results.size(); i++) {
        Appointment &app = results[i].first;
        ui->tableQuery->setItem(i, 0, new QTableWidgetItem(app.name));
        ui->tableQuery->setItem(i, 1, new QTableWidgetItem(app.phone));
        ui->tableQuery->setItem(i, 2, new QTableWidgetItem(app.gender));
        ui->tableQuery->setItem(i, 3, new QTableWidgetItem(QString::number(app.age)));
        ui->tableQuery->setItem(i, 4, new QTableWidgetItem(app.appointTime));
        ui->tableQuery->setItem(i, 5, new QTableWidgetItem(results[i].second));
    }
}

// ========== 保存数据 ==========
void MainWindow::saveData() {
    QFile file("hospital_data.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);


    for (int i = 0; i < clinicList.size(); i++) {
        Clinic &c = clinicList[i];
        out << "CLINIC:"
            << c.clinicNo << "|" << c.clinicName << "|"
            << c.doctorNo << "|" << c.consultTime << "|"
            << c.capacity << "|" << c.address << "|" << c.phone << "\n";

        for (int j = 0; j < c.appointments.size(); j++) {
            Appointment &a = c.appointments[j];
            out << "APPT:"
                << a.name << "|" << a.phone << "|"
                << a.gender << "|" << a.age << "|" << a.appointTime << "\n";
        }
    }
    file.close();
}

// ========== 加载数据 ==========
void MainWindow::loadData() {
    QFile file("hospital_data.txt");
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);

    Clinic *currentClinic = nullptr;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("CLINIC:")) {
            QStringList f = line.mid(7).split("|");
            if (f.size() >= 7) {
                Clinic c;
                c.clinicNo = f[0]; c.clinicName = f[1]; c.doctorNo = f[2];
                c.consultTime = f[3]; c.capacity = f[4].toInt();
                c.address = f[5]; c.phone = f[6];
                c.currentCount = 0;
                clinicList.append(c);
                currentClinic = &clinicList.last();
            }
        } else if (line.startsWith("APPT:") && currentClinic) {
            QStringList f = line.mid(5).split("|");
            if (f.size() >= 5) {
                Appointment a;
                a.name = f[0]; a.phone = f[1]; a.gender = f[2];
                a.age = f[3].toInt(); a.appointTime = f[4];
                currentClinic->appointments.append(a);
                currentClinic->currentCount++;
            }
        }
    }
    file.close();
}