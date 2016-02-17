#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "trialsetup.h"
#include "patient.h"

#include <QMessageBox>
#include <QLabel>
#include <QSerialPort>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serial = new QSerialPort(this);

    settings = new SettingsDialog;
    trialSetup = new TrialSetup;
    patient = new Patient;

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(false);

    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
            SLOT(handleError(QSerialPort::SerialPortError)));
    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));

}

MainWindow::~MainWindow()
{
    delete settings;
    delete trialSetup;
    delete patient;
    delete ui;
    // Check if i need to close serial here
}

void MainWindow::openSerialPort()
{    
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        serial->clear();
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate)
                          .arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits)
                          .arg(p.stringFlowControl));
        if (serial->isWritable()){
            QTimer::singleShot(1500,this,SLOT(initialFirmwareSetup()));
            ui->actionStart->setEnabled(true);
        }else qDebug() << "Serial is not writable yet!";

    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
        showStatusMessage(tr("Open serial port error"));
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Non contact tremor data logger"),
                       tr("<b>Tremor data logger</b> is a software to hand motions "
                          "record using an Arduino Due, with a specific shield "
                          "used to handle the Plessey sensor PS25454."));
}

void MainWindow::writeData(const QByteArray &data)
{
    qint64 w = serial->write(data);//, qstrlen(data));
    serial->flush();
}

void MainWindow::readData()
{
    //    QByteArray data = serial->readAll();
    while(serial->canReadLine()){
        QByteArray data = serial->readLine();
        qDebug() << data.data();
    }
    //Save received data at this point
}

void MainWindow::initialFirmwareSetup()
{
    TrialSetup::TrialSetupConfig ts = trialSetup->trialSetupConfig();
    QByteArray arduinoSetup;
    QTextStream(&arduinoSetup) << "|" << ts.outputSignal[0] << ts.outputSignal[1]
                               << ts.outputSignal[2] << "|" << ts.sampleRate << "|#" << endl;
    writeData(arduinoSetup);
}

void MainWindow::startDataCollection()
{
    QByteArray ba = "r";
    writeData(ba);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(true);
    showStatusMessage(tr("Data collection started"));
}

void MainWindow::stopDataCollection()
{
    QByteArray ba = "2";
    writeData(ba);
    showStatusMessage(tr("Stop in progress"));
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(closeSerialPort()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionConfigure, SIGNAL(triggered()), settings, SLOT(show()));
    connect(ui->actionTrialSetup, SIGNAL(triggered()), trialSetup, SLOT(show()));
    connect(ui->actionPatient, SIGNAL(triggered()), patient, SLOT(show()));
    connect(ui->actionStart, SIGNAL(triggered()), this, SLOT(startDataCollection()));
    connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(stopDataCollection()));
    //connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}
