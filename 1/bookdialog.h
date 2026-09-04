#ifndef BOOKDIALOG_H
#define BOOKDIALOG_H

#include <QDialog>
#include <QList>
#include "clinic.h"
#include "appointment.h"

namespace Ui {
class BookDialog;
}

class BookDialog : public QDialog {
    Q_OBJECT

public:
    explicit BookDialog(const QList<Clinic> &clinics, QWidget *parent = nullptr);
    ~BookDialog();

    int getSelectedClinicIndex() const;
    Appointment getAppointment() const;

private slots:
    void on_btnOk_clicked();
    void on_btnCancel_clicked();

private:
    Ui::BookDialog *ui;
    QList<Clinic> clinicList;
    int selectedIndex;
    Appointment resultAppoint;
};

#endif // BOOKDIALOG_H
