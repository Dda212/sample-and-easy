// appointment.h（预约信息类）
#ifndef APPOINTMENT_H
#define APPOINTMENT_H
#include <QString>

class Appointment {
public:
    QString name;        // 预约人姓名
    QString phone;       // 电话
    QString gender;      // 性别
    int age;             // 年龄
    QString appointTime; // 预约时间

    Appointment(QString n="", QString p="", QString g="", int a=0, QString t="");
};
#endif