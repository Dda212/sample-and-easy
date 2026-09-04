#include "clinic.h"

Clinic::Clinic(QString no, QString name, QString dNo,
               QString time, int cap, QString addr, QString tel)
    : clinicNo(no), clinicName(name), doctorNo(dNo),
    consultTime(time), capacity(cap), address(addr), phone(tel)
{
}

bool Clinic::canBook() const {
    return appointments.size() < capacity;
}

bool Clinic::addAppointment(const Appointment& app) {
    if (!canBook()) {
        return false;  // 已满，无法预约
    }
    appointments.append(app);
    return true;
}

bool Clinic::removeAppointment(int index) {
    if (index < 0 || index >= appointments.size()) {
        return false;
    }
    appointments.removeAt(index);
    return true;
}

// 数据加载用：绕过容量校验直接追加（加载历史数据时可能出现已预约数>=容量的情况）
void Clinic::addAppointmentRaw(const Appointment& app) {
    appointments.append(app);
}

// 批量设置预约列表（编辑门诊时保留原预约数据用）
void Clinic::setAppointments(const QList<Appointment>& apps) {
    appointments = apps;
}

int Clinic::getAvailableSlots() const {
    return capacity - appointments.size();
}

int Clinic::getCurrentCount() const {
    return appointments.size();
}
