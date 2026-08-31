#include "appointment.h"

Appointment::Appointment(QString n, QString p, QString g, int a, QString t)
    : name(n), phone(p), gender(g), age(a), appointTime(t)
{
}