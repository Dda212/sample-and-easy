// clinic.h（门诊信息类）
#ifndef CLINIC_H
#define CLINIC_H
#include <QString>
#include <QList>
#include "appointment.h"

class Clinic {
public:
    QString clinicNo;         // 门诊号
    QString clinicName;       // 门诊名称
    QString doctorNo;         // 值班医生工号
    QString consultTime;      // 接诊时间
    int capacity;             // 容量
    int currentCount;         // 当前已预约人数
    QString address;          // 门诊地址
    QString phone;            // 联系电话
    QList<Appointment> appointments; // 该门诊的预约列表（1对N）

    Clinic(QString no="", QString name="", QString dNo="", QString time="",
           int cap=0, QString addr="", QString tel="");

    bool canBook() const;                          // 是否还能预约（容量是否够）
    bool addAppointment(const Appointment& app);   // 添加预约
    int getAvailableSlots() const;            // 获取剩余可预约号源数
};
#endif