#ifndef PATIENTDATA_H
#define PATIENTDATA_H

#include <QDialog>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class PatientData;
}

QT_END_NAMESPACE

class PatientData : public QDialog
{
    Q_OBJECT

public:


    explicit PatientData(QWidget *parent = 0);
    ~PatientData();

private:
    Ui::PatientData *ui;
};

#endif // PATIENTDATA_H
