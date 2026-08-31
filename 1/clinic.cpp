#include "clinic.h"

Clinic::Clinic(QString no, QString name, QString dNo,
               QString time, int cap, QString addr, QString tel)
    : clinicNo(no), clinicName(name), doctorNo(dNo),
    consultTime(time), capacity(cap), currentCount(0),
    address(addr), phone(tel)
{
}

bool Clinic::canBook() const {
    return currentCount < capacity;
}

bool Clinic::addAppointment(const Appointment& app) {
    if (!canBook()) {
        return false;  // 已满，无法预约
    }
    appointments.append(app);
    currentCount++;
    return true;
}

int Clinic::getAvailableSlots() const {
    return capacity - currentCount;
}