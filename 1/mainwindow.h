#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QHash>
#include <QPair>
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
    void on_btnEditClinic_clicked();    // 编辑门诊
    void on_btnDeleteClinic_clicked();  // 删除门诊
    void on_btnBook_clicked();          // 预约门诊
    void on_btnQuery_clicked();         // 查询预约
    void on_btnCancelAppoint_clicked(); // 取消预约
    void on_btnExit_clicked();          // 退出系统
    void on_comboSearchClinic_currentIndexChanged(int index);
    void on_editSearchPhone_returnPressed();

private:
    Ui::MainWindow *ui;
    QList<Clinic> clinicList;           // 存储所有门诊数据
    QHash<QString, int> m_clinicIndex;  // 门诊号 → 下标索引（O(1) 查找）
    QList<QPair<int,int>> m_queryRowMap; // 查询表格行 →（门诊下标, 预约下标）

    void rebuildClinicIndex();          // 重建门诊号索引
    void updateStatusBar();              // 更新状态栏统计信息
    void refreshClinicTable();          // 刷新门诊表格显示
    void refreshQueryByClinic(int clinicIdx);       // 按门诊刷新查询结果
    void refreshQueryByPhone(const QString &phone); // 按电话刷新查询结果
    void fillQueryCombo();              // 填充查询区域的门诊下拉框
    void reapplyQuery();                // 按当前查询方式重新刷新查询结果
    void saveData(bool silent = false);       // 保存数据到 JSON 文件（原子写入，silent=true 时不弹错误框）
    void loadData();                    // 从 JSON 文件加载数据（自动迁移旧 txt 格式）
    void loadLegacyTxt(const QString &path); // 读取旧版 txt 数据（仅迁移用）
};

#endif // MAINWINDOW_H
