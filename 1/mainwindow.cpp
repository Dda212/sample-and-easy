#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addclinicdialog.h"
#include "bookdialog.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 让主布局随窗口缩放（修复固定尺寸布局不跟随窗口变化）
    ui->layoutWidget->setMinimumSize(0, 0);
    ui->layoutWidget->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ui->layoutWidget->setGeometry(QRect(0, 0, 0, 0));
    auto *centralLayout = new QVBoxLayout(ui->centralwidget);
    centralLayout->setContentsMargins(12, 12, 12, 12);
    centralLayout->setSpacing(8);
    ui->layoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    centralLayout->addWidget(ui->layoutWidget);

    // 表格列宽自适应
    ui->tableClinics->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableQuery->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 按钮水平自适应：等比拉伸 + 最小宽度归零 + 布局间距收紧
    ui->horizontalLayout->setSpacing(8);
    ui->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    QList<QPushButton*> btns = {ui->btnViewClinics, ui->btnAddClinic, ui->btnBook, ui->btnQuery, ui->btnExit};
    for (auto *btn : btns) {
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setMinimumWidth(0);
        btn->setMaximumWidth(QWIDGETSIZE_MAX);
    }
    for (int i = 0; i < ui->horizontalLayout->count(); ++i)
        ui->horizontalLayout->setStretch(i, 1);

    resize(1200, 700);

    loadData();
    refreshClinicTable();   // 启动即显示已有门诊数据
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
        saveData();   // 立即持久化，避免异常退出丢失
        QMessageBox::information(this, "成功", "门诊添加成功！");
    }
}

// ========== 预约门诊 ==========
void MainWindow::on_btnBook_clicked() {
    if (clinicList.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有门诊信息，请先添加门诊！");
        return;
    }

    // 检查是否有可预约的门诊
    bool anyBookable = false;
    for (const Clinic &c : clinicList) {
        if (c.canBook()) { anyBookable = true; break; }
    }
    if (!anyBookable) {
        QMessageBox::warning(this, "提示", "当前所有门诊均已满，无法预约！");
        return;
    }

    BookDialog dialog(clinicList, this);
    if (dialog.exec() == QDialog::Accepted) {
        int idx = dialog.getSelectedClinicIndex();
        if (idx < 0 || idx >= clinicList.size()) {
            QMessageBox::warning(this, "预约失败", "未选择有效的门诊！");
            return;
        }
        Appointment app = dialog.getAppointment();

        // 重复预约检查：同一电话不可重复预约同一门诊
        for (const Appointment &existing : clinicList[idx].appointments) {
            if (existing.phone == app.phone) {
                QMessageBox::warning(this, "预约失败",
                                     "该电话已预约过此门诊，不可重复预约！");
                return;
            }
        }

        bool success = clinicList[idx].addAppointment(app);
        if (success) {
            QMessageBox::information(this, "预约成功",
                                     "已成功预约：" + clinicList[idx].clinicName
                                         + "\n预约人：" + app.name);
            refreshClinicTable();
            saveData();   // 立即持久化
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
    if (QMessageBox::question(this, "确认退出", "确定要退出系统吗？",
                               QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    saveData();
    this->close();
}

// ========== 按门诊下拉框查询 ==========
void MainWindow::on_comboSearchClinic_currentIndexChanged(int index) {
    // index < 0（如 clear() 触发）时清空查询结果，避免误填第一个门诊
    if (index < 0) {
        ui->tableQuery->setRowCount(0);
        return;
    }
    int clinicIdx = ui->comboSearchClinic->itemData(index).toInt();
    if (clinicIdx >= 0 && clinicIdx < clinicList.size()) {
        refreshQueryByClinic(clinicIdx);
    } else {
        ui->tableQuery->setRowCount(0);   // 选中"-- 按门诊查询 --"占位项时清空
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
    QFile file(QCoreApplication::applicationDirPath() + "/hospital_data.txt");
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
    QFile file(QCoreApplication::applicationDirPath() + "/hospital_data.txt");
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

    // 防御：若加载到的预约数超过容量，自动将容量补齐，避免剩余号源为负
    for (Clinic &c : clinicList) {
        if (c.currentCount > c.capacity) c.capacity = c.currentCount;
    }
}
