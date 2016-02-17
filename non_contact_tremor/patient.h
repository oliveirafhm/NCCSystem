#ifndef PATIENT_H
#define PATIENT_H

#include <QDialog>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class Patient;
}

QT_END_NAMESPACE

class Patient : public QDialog
{
    Q_OBJECT

public:
    struct PatientData{
        QString name = "";
        QString gender = "";
        quint8 age = 0;
    };

    explicit Patient(QWidget *parent = 0);
    ~Patient();

    PatientData patientData() const;

public slots:
    void show();

private slots:
    void apply();

private:
    void fillUI();
    void updatePatientData();

private:
    Ui::Patient *ui;
    PatientData currentPatientData;
};

#endif // PATIENT_H
