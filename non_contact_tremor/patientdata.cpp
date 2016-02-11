#include "patientdata.h"
#include "ui_patientdata.h"

PatientData::PatientData(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatientData)
{
    ui->setupUi(this);
}

PatientData::~PatientData()
{
    delete ui;
}
