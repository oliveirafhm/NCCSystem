#include "trialsetup.h"
#include "ui_trialsetup.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

QT_USE_NAMESPACE

TrialSetup::TrialSetup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TrialSetup)
{
    ui->setupUi(this);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(apply()));

    fillUI();
}

TrialSetup::~TrialSetup()
{
    delete ui;
}

void TrialSetup::on_chooseDirectoryButton_clicked()
{
    QString directoryPath = QFileDialog::getExistingDirectory(
                this,
                tr("Select directory"),
                "/"
                );
    QMessageBox::information(this,tr("Directory path"), directoryPath);
    ui->directoryPathLineEdit->setText(directoryPath);
}

TrialSetup::TrialSetupConfig TrialSetup::trialSetupConfig() const
{
    return currentTrialSetup;
}

void TrialSetup::apply()
{
    updateTrialSetupConfig();
    accept();

}

void TrialSetup::updateTrialSetupConfig()
{
    qDebug() << "update called!";
}

void TrialSetup::fillUI()
{
    ui->analogOutSignalCheckBox->setChecked(static_cast<bool>(currentTrialSetup.outputSignal[0]));
    ui->digitalOutSignalACheckBox->setChecked(static_cast<bool>(currentTrialSetup.outputSignal[1]));
    ui->digitalOutSignalBCheckBox->setChecked(static_cast<bool>(currentTrialSetup.outputSignal[2]));

    ui->sampleRateLineEdit->setText(QString::number(currentTrialSetup.sampleRate));
    ui->collectionTimeoutLineEdit->setText(QString::number(currentTrialSetup.collectionTimeOut));
    ui->directoryPathLineEdit->setText(currentTrialSetup.directoryPath);
}
