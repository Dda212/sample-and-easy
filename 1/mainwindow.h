#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include "clinic.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnViewClinics_clicked();   // 查看所有门诊
    void on_btnAddClinic_clicked();     // 添加门诊
    void on_btnBook_clicked();          // 预约门诊
    void on_btnQuery_clicked();         // 查询预约
    void on_btnExit_clicked();          // 退出系统
    void on_comboSearchClinic_currentIndexChanged(int index);
    void on_editSearchPhone_returnPressed();

private:
    Ui::MainWindow *ui;
    QList<Clinic> clinicList;           // 存储所有门诊数据

    void refreshClinicTable();          // 刷新门诊表格显示
    void refreshQueryByClinic(int clinicIdx);       // 按门诊刷新查询结果
    void refreshQueryByPhone(const QString &phone); // 按电话刷新查询结果
    void saveData();                    // 保存数据到文件
    void loadData();                    // 从文件加载数据
};

#endif // MAINWINDOW_H