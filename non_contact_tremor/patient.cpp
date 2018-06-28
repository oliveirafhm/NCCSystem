#include "patient.h"
#include "ui_patient.h"

#include<QDebug>

Patient::Patient(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Patient)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(apply()));
}

Patient::~Patient()
{
    delete ui;
}

Patient::PatientData Patient::patientData() const
{
    return currentPatientData;
}

void Patient::show()
{
    fillUI();
    QWidget::show();
}

void Patient::apply()
{
    updatePatientData();
    accept();
}

void Patient::updatePatientData()
{
    currentPatientData.name = ui->nameLineEdit->text();
    currentPatientData.gender = ui->genderComboBox->currentText();
//    currentPatientData.birthdate = ui->dateEdit->date();

//    qDebug() << "updatePatientData";
//    qDebug() << currentPatientData.gender << "Current";
//    qDebug() << ui->genderComboBox->currentText() << "From combo";
}

void Patient::fillUI()
{
    ui->nameLineEdit->setText(currentPatientData.name);
    qint8 gender = -1;
//    qDebug() << "fillUI";
    if(currentPatientData.gender == QString("")){
        gender = 0;
//        qDebug() << "Empty";
    }
    else if(currentPatientData.gender == QString("Male")){
        gender = 1;
//        qDebug() << "Male";
    }
    else if(currentPatientData.gender == QString("Female")){
        gender = 2;
//        qDebug() << "Female";
    }
    ui->genderComboBox->setCurrentIndex(gender);
//    ui->dateEdit->setDate(currentPatientData.birthdate);
}
