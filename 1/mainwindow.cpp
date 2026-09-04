#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addclinicdialog.h"
#include "bookdialog.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QSaveFile>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // 表格表头列宽均匀拉伸占满行宽（.ui 不支持此属性，需代码设置）
    ui->tableClinics->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableQuery->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    loadData();
    rebuildClinicIndex();
    refreshClinicTable();
    updateStatusBar();
}

MainWindow::~MainWindow() {
    saveData(true);  // 析构时静默保存，不弹 QMessageBox
    delete ui;
}

// ========== 重建门诊号索引（门诊号 → 下标，O(1) 查找） ==========
void MainWindow::rebuildClinicIndex() {
    m_clinicIndex.clear();
    for (int i = 0; i < clinicList.size(); i++) {
        m_clinicIndex.insert(clinicList[i].clinicNo, i);
    }
}

// ========== 更新状态栏统计信息 ==========
void MainWindow::updateStatusBar() {
    int totalClinics = clinicList.size();
    int totalAppointments = 0;
    int totalCapacity = 0;
    for (const Clinic &c : clinicList) {
        totalAppointments += c.getCurrentCount();
        totalCapacity += c.capacity;
    }
    int available = totalCapacity - totalAppointments;
    statusBar()->showMessage(QString("共 %1 个门诊  |  总容量 %2  |  已预约 %3  |  剩余号源 %4")
                                  .arg(totalClinics).arg(totalCapacity).arg(totalAppointments).arg(available));
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

        // O(1) 重复号检查
        if (m_clinicIndex.contains(newClinic.clinicNo)) {
            QMessageBox::warning(this, "添加失败",
                                 "门诊号 " + newClinic.clinicNo + " 已存在！");
            return;
        }

        clinicList.append(newClinic);
        m_clinicIndex.insert(newClinic.clinicNo, clinicList.size() - 1);
        refreshClinicTable();
        saveData();
        updateStatusBar();
        QMessageBox::information(this, "成功", "门诊添加成功！");
    }
}

// ========== 编辑门诊 ==========
void MainWindow::on_btnEditClinic_clicked() {
    int row = ui->tableClinics->currentRow();
    if (row < 0 || row >= clinicList.size()) {
        QMessageBox::warning(this, "提示", "请先在门诊列表中选中要编辑的门诊！");
        return;
    }

    Clinic &clinic = clinicList[row];

    AddClinicDialog dialog(this);
    dialog.setEditClinic(clinic);
    if (dialog.exec() == QDialog::Accepted) {
        Clinic edited = dialog.getClinic();
        edited.clinicNo = clinic.clinicNo;       // 门诊号不可修改（对话框中已只读，双保险）

        // 容量不允许改得比已预约人数还小
        if (edited.capacity < clinic.getCurrentCount()) {
            QMessageBox::warning(this, "编辑失败",
                                 "容量不能小于当前已预约人数（"
                                 + QString::number(clinic.getCurrentCount()) + "）！");
            return;
        }

        // 保留原门诊的预约数据，仅覆盖可编辑字段
        edited.setAppointments(clinic.appointments);
        clinicList[row] = edited;
        refreshClinicTable();
        fillQueryCombo();
        saveData();
        updateStatusBar();
        QMessageBox::information(this, "成功", "门诊信息已更新！");
    }
}

// ========== 删除门诊 ==========
void MainWindow::on_btnDeleteClinic_clicked() {
    int row = ui->tableClinics->currentRow();
    if (row < 0 || row >= clinicList.size()) {
        QMessageBox::warning(this, "提示", "请先在门诊列表中选中要删除的门诊！");
        return;
    }

    const Clinic &clinic = clinicList[row];
    QString warningMsg = "确定要删除门诊「" + clinic.clinicName + "（" + clinic.clinicNo + "）」吗？";
    if (!clinic.appointments.isEmpty()) {
        warningMsg += "\n\n注意：该门诊还有 " + QString::number(clinic.appointments.size())
                    + " 条预约记录，将一并删除！";
    }

    if (QMessageBox::question(this, "确认删除", warningMsg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    clinicList.removeAt(row);
    rebuildClinicIndex();   // 删除后下标变化，重建索引
    refreshClinicTable();
    fillQueryCombo();
    reapplyQuery();
    saveData();
    updateStatusBar();
    QMessageBox::information(this, "成功", "门诊已删除！");
}

// ========== 预约门诊 ==========
void MainWindow::on_btnBook_clicked() {
    if (clinicList.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有门诊信息，请先添加门诊！");
        return;
    }

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
            reapplyQuery();
            saveData();
            updateStatusBar();
        } else {
            QMessageBox::warning(this, "预约失败", "该门诊已满，无法预约！");
        }
    }
}

// ========== 取消预约 ==========
void MainWindow::on_btnCancelAppoint_clicked() {
    if (!ui->queryContainer->isVisible()) {
        on_btnQuery_clicked();
    }

    int row = ui->tableQuery->currentRow();
    if (row < 0 || row >= m_queryRowMap.size()) {
        QMessageBox::warning(this, "提示",
                             "请先在下方查询结果表格中选中要取消的预约记录！\n"
                             "（提示：可点击「查询预约」按钮展开查询区域）");
        return;
    }

    int clinicIdx = m_queryRowMap[row].first;
    int appIdx = m_queryRowMap[row].second;
    if (clinicIdx < 0 || clinicIdx >= clinicList.size()
        || appIdx < 0 || appIdx >= clinicList[clinicIdx].appointments.size()) {
        QMessageBox::warning(this, "提示", "该记录已失效，请重新查询后再试！");
        return;
    }

    Clinic &clinic = clinicList[clinicIdx];
    const Appointment &app = clinic.appointments[appIdx];
    QString msg = "确定要取消以下预约吗？\n"
                  "门诊：" + clinic.clinicName +
                  "\n预约人：" + app.name +
                  "\n电话：" + app.phone +
                  "\n预约时间：" + app.appointTime;
    if (QMessageBox::question(this, "取消预约", msg,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    clinic.removeAppointment(appIdx);   // 封装方法：移除预约并释放号源
    m_queryRowMap.clear();

    refreshClinicTable();
    reapplyQuery();
    saveData();
    updateStatusBar();
    QMessageBox::information(this, "成功", "预约已取消，号源已释放！");
}

// ========== 查询预约（切换显示/隐藏） ==========
void MainWindow::on_btnQuery_clicked() {
    bool isVisible = ui->queryContainer->isVisible();

    if (!isVisible) {
        ui->queryContainer->setVisible(true);

        ui->comboSearchClinic->clear();
        ui->comboSearchClinic->addItem("-- 按门诊查询 --", -1);
        for (int i = 0; i < clinicList.size(); i++) {
            ui->comboSearchClinic->addItem(clinicList[i].clinicName, i);
        }
    } else {
        ui->queryContainer->setVisible(false);
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
    if (index < 0) {
        ui->tableQuery->setRowCount(0);
        m_queryRowMap.clear();
        return;
    }
    int clinicIdx = ui->comboSearchClinic->itemData(index).toInt();
    if (clinicIdx >= 0 && clinicIdx < clinicList.size()) {
        refreshQueryByClinic(clinicIdx);
    } else {
        ui->tableQuery->setRowCount(0);
        m_queryRowMap.clear();
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
        ui->tableClinics->setItem(i, 5, new QTableWidgetItem(QString::number(clinicList[i].getCurrentCount())));
        ui->tableClinics->setItem(i, 6, new QTableWidgetItem(QString::number(clinicList[i].getAvailableSlots())));
    }
}

// ========== 按门诊刷新查询结果 ==========
void MainWindow::refreshQueryByClinic(int clinicIdx) {
    m_queryRowMap.clear();
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
        m_queryRowMap.append(qMakePair(clinicIdx, i));
    }
}

// ========== 按电话刷新查询结果 ==========
void MainWindow::refreshQueryByPhone(const QString &phone) {
    m_queryRowMap.clear();

    // 先收集匹配记录（通过 m_queryRowMap 定位，无需额外 results 列表）
    for (int i = 0; i < clinicList.size(); i++) {
        for (int j = 0; j < clinicList[i].appointments.size(); j++) {
            if (clinicList[i].appointments[j].phone == phone) {
                m_queryRowMap.append(qMakePair(i, j));
            }
        }
    }

    ui->tableQuery->setRowCount(m_queryRowMap.size());
    if (m_queryRowMap.isEmpty()) {
        QMessageBox::information(this, "查询结果", "未找到该电话的预约记录。");
        return;
    }

    for (int i = 0; i < m_queryRowMap.size(); i++) {
        int cIdx = m_queryRowMap[i].first;
        int aIdx = m_queryRowMap[i].second;
        const Appointment &app = clinicList[cIdx].appointments[aIdx];
        ui->tableQuery->setItem(i, 0, new QTableWidgetItem(app.name));
        ui->tableQuery->setItem(i, 1, new QTableWidgetItem(app.phone));
        ui->tableQuery->setItem(i, 2, new QTableWidgetItem(app.gender));
        ui->tableQuery->setItem(i, 3, new QTableWidgetItem(QString::number(app.age)));
        ui->tableQuery->setItem(i, 4, new QTableWidgetItem(app.appointTime));
        ui->tableQuery->setItem(i, 5, new QTableWidgetItem(clinicList[cIdx].clinicName));
    }
}

// ========== 填充查询区域的门诊下拉框 ==========
void MainWindow::fillQueryCombo() {
    if (!ui->queryContainer->isVisible()) return;
    QVariant currentData = ui->comboSearchClinic->currentData();
    ui->comboSearchClinic->clear();
    ui->comboSearchClinic->addItem("-- 按门诊查询 --", -1);
    for (int i = 0; i < clinicList.size(); i++) {
        ui->comboSearchClinic->addItem(clinicList[i].clinicName, i);
    }
    if (currentData.isValid()) {
        int restoreIndex = ui->comboSearchClinic->findData(currentData);
        if (restoreIndex >= 0) {
            ui->comboSearchClinic->setCurrentIndex(restoreIndex);
        }
    }
}

// ========== 按当前查询方式重新刷新查询结果 ==========
void MainWindow::reapplyQuery() {
    if (!ui->queryContainer->isVisible()) return;

    QString phone = ui->editSearchPhone->text().trimmed();
    if (!phone.isEmpty()) {
        refreshQueryByPhone(phone);
        return;
    }
    int clinicIdx = ui->comboSearchClinic->currentData().toInt();
    if (clinicIdx >= 0 && clinicIdx < clinicList.size()) {
        refreshQueryByClinic(clinicIdx);
    } else {
        ui->tableQuery->setRowCount(0);
    }
}

// ========== 保存数据（JSON 格式，QSaveFile 原子写入） ==========
void MainWindow::saveData(bool silent) {
    QJsonArray clinicsArr;
    for (const Clinic &c : clinicList) {
        QJsonObject clinicObj;
        clinicObj["clinicNo"]     = c.clinicNo;
        clinicObj["clinicName"]   = c.clinicName;
        clinicObj["doctorNo"]     = c.doctorNo;
        clinicObj["consultTime"]  = c.consultTime;
        clinicObj["capacity"]     = c.capacity;
        clinicObj["address"]      = c.address;
        clinicObj["phone"]        = c.phone;

        QJsonArray appsArr;
        for (const Appointment &a : c.appointments) {
            QJsonObject appObj;
            appObj["name"]        = a.name;
            appObj["phone"]       = a.phone;
            appObj["gender"]      = a.gender;
            appObj["age"]         = a.age;
            appObj["appointTime"] = a.appointTime;
            appsArr.append(appObj);
        }
        clinicObj["appointments"] = appsArr;
        clinicsArr.append(clinicObj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["clinics"] = clinicsArr;

    // QSaveFile：先写临时文件，commit() 时原子 rename，防止写入中断导致数据损坏
    const QString filePath = QCoreApplication::applicationDirPath() + "/hospital_data.json";
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (!silent) {
            QMessageBox::critical(this, "保存失败",
                                  "数据保存失败：无法写入文件！\n"
                                  + filePath + "\n" + file.errorString());
        }
        return;
    }
    file.write(QJsonDocument(root).toJson());
    if (!file.commit()) {
        if (!silent) {
            QMessageBox::critical(this, "保存失败",
                                  "数据保存失败：原子提交失败！\n" + file.errorString());
        }
    }
}

// ========== 加载数据（JSON 格式，自动迁移旧 txt） ==========
void MainWindow::loadData() {
    const QString jsonPath = QCoreApplication::applicationDirPath() + "/hospital_data.json";
    const QString txtPath  = QCoreApplication::applicationDirPath() + "/hospital_data.txt";

    if (QFile::exists(jsonPath)) {
        QFile file(jsonPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QMessageBox::warning(this, "数据加载",
                                 "数据文件解析失败，将以空数据启动。\n"
                                 "错误：" + err.errorString());
            return;
        }

        const QJsonArray clinicsArr = doc.object().value("clinics").toArray();
        for (const QJsonValue &cv : clinicsArr) {
            QJsonObject co = cv.toObject();
            Clinic c;
            c.clinicNo    = co.value("clinicNo").toString();
            c.clinicName  = co.value("clinicName").toString();
            c.doctorNo    = co.value("doctorNo").toString();
            c.consultTime = co.value("consultTime").toString();
            c.capacity    = co.value("capacity").toInt();
            c.address     = co.value("address").toString();
            c.phone       = co.value("phone").toString();

            const QJsonArray appsArr = co.value("appointments").toArray();
            for (const QJsonValue &av : appsArr) {
                QJsonObject ao = av.toObject();
                Appointment a;
                a.name        = ao.value("name").toString();
                a.phone       = ao.value("phone").toString();
                a.gender      = ao.value("gender").toString();
                a.age         = ao.value("age").toInt();
                a.appointTime = ao.value("appointTime").toString();
                c.addAppointmentRaw(a);   // 加载用：绕过容量校验
            }
            clinicList.append(c);
        }
    } else if (QFile::exists(txtPath)) {
        loadLegacyTxt(txtPath);
        saveData();
    }

    // 防御：若加载到的预约数超过容量，自动将容量补齐
    for (Clinic &c : clinicList) {
        if (c.getCurrentCount() > c.capacity) c.capacity = c.getCurrentCount();
    }
}

// ========== 读取旧版 txt 数据（仅迁移用） ==========
void MainWindow::loadLegacyTxt(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);  // 显式指定 UTF-8，避免默认编码不一致
    int currentClinicIdx = -1;   // 用索引代替指针，避免 QList 重新分配时指针悬空

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
                clinicList.append(c);
                currentClinicIdx = clinicList.size() - 1;
            }
        } else if (line.startsWith("APPT:") && currentClinicIdx >= 0) {
            QStringList f = line.mid(5).split("|");
            if (f.size() >= 5) {
                Appointment a;
                a.name = f[0]; a.phone = f[1]; a.gender = f[2];
                a.age = f[3].toInt(); a.appointTime = f[4];
                clinicList[currentClinicIdx].addAppointmentRaw(a);
            }
        }
    }
    file.close();
}
