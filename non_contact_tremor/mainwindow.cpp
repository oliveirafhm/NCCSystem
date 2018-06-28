#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "trialsetup.h"
#include "patient.h"

#include <QMessageBox>
#include <QLabel>
#include <QSerialPort>
//#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    serial = new QSerialPort(this);
    _ba = new QByteArray();

    settings = new SettingsDialog;
    trialSetup = new TrialSetup;
    patient = new Patient;

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(false);
    ui->actionSave->setEnabled(false);
    ui->actionPlotSignals->setEnabled(false);

    qint8 iconStatusHeight = ui->statusBar->height()/1.2;
    pixmapS0 = new QPixmap(QPixmap(":/images/rec_neutral.png").scaledToHeight(iconStatusHeight));
    pixmapS1 = new QPixmap(QPixmap(":/images/rec_red.png").scaledToHeight(iconStatusHeight));
    pixmapS2 = new QPixmap(QPixmap(":/images/rec_yellow.png").scaledToHeight(iconStatusHeight));
    pixmapS3 = new QPixmap(QPixmap(":/images/rec_green.png").scaledToHeight(iconStatusHeight));

    status = new QLabel;
    iconStatus = new QLabel;
    ui->statusBar->addWidget(iconStatus);
    ui->statusBar->addWidget(status);
    changeIconStatus(0);

    showStatusMessage(tr("Ok"), StatusFlag::Init);
//    TODO: Failed attempts to scroll down serial monitor text while it is updated
//    ui->serialMonitorTextEdit->ensureCursorVisible();
//    ui->serialMonitorTextEdit->setCenterOnScroll(true);

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
    // TODO: Check if i need to close/delete serial here
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
                          .arg(p.stringFlowControl), StatusFlag::Connected);
        if (serial->isWritable()){
            QTimer::singleShot(1500,this,SLOT(initialFirmwareSetup()));
            ui->actionStart->setEnabled(true);
            ui->actionTrialSetup->setEnabled(false);
            changeIconStatus(2);
        }else showStatusMessage(tr("Serial is not writable yet!"), StatusFlag::Unknown);

    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());
        showStatusMessage(tr("Open serial port error"), StatusFlag::Unknown);
    }
}

void MainWindow::closeSerialPort()
{
    showStatusMessage(tr("Disconnected"), StatusFlag::Disconnected);
    // Reset
    QByteArray ba = "n";
    writeData(ba);

    if (serial->isOpen())
        serial->close();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(false);
    ui->actionPlotSignals->setEnabled(false);
    ui->actionTrialSetup->setEnabled(true);    
    changeIconStatus(1);
    ui->serialMonitorTextEdit->clear();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Non contact sensor data logger"),
                       tr("Non contact sensor data logger is a software to record "
                          "and plot signals from non-contact capacitive sensors (PS25454). "
                          "There is a specific firmware to use along with this software. "
                          "Check it with the author: Fábio Henrique (oliveirafhm@gmail.com)"));
}

qint64 MainWindow::writeData(const QByteArray &data)
{
    qint64 w = serial->write(data);//, qstrlen(data));
    serial->flush();
    return w;
}

qint64 MainWindow::writeData()
{
    qint64 w = serial->write(*_ba);//, qstrlen(data));
    serial->flush();
    _ba->clear();
    return w;
}

void MainWindow::readData()
{    
    while(serial->canReadLine()){
        QByteArray data = serial->readLine();
        if (statusFlag != StatusFlag::Saving){
            ui->serialMonitorTextEdit->insertPlainText(data.data());
            if (statusFlag == StatusFlag::Stopped){
                stopDataHandler(data.data());
            }
        } else{
            saveDataHandle(data.data());            
        }
    }
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
    showStatusMessage(tr("Data collection started"), StatusFlag::Recording);
    QByteArray ba = "r";
    writeData(ba);
    // Start data collection after 1.5s
    *_ba = "1";
    QTimer::singleShot(1500,this,SLOT(writeData()));
    ui->actionSave->setEnabled(false);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionPlotSignals->setEnabled(false);
    changeIconStatus(3);
}

void MainWindow::stopDataCollection()
{
    showStatusMessage(tr("Stopped (wait while the file conversion occurs)"), StatusFlag::Stopped);
    QByteArray ba = "2";
    writeData(ba);
    ui->actionStop->setEnabled(false);
    changeIconStatus(2); 
}

void MainWindow::stopDataHandler(char *data){
    if (QString(data).contains("Done", Qt::CaseInsensitive)){
        ui->actionSave->setEnabled(true);
        ui->actionDisconnect->setEnabled(true);
        ui->actionStart->setEnabled(true);
        showStatusMessage(tr("CSV file successfully created in SD Card!"), StatusFlag::ConversionDone);
    }
}

void MainWindow::saveDataCollection()
{
    showStatusMessage(tr("Saving... (wait while the file is saved)"), StatusFlag::Saving);
    QByteArray ba = "d";
    writeData(ba);    
    ui->actionSave->setEnabled(false);
    // Create file and open it to write
}

void MainWindow::saveDataHandle(char *data)
{
    //Create header file
    // Date and time | Plesse csv file name
    //    qDebug() << "Save data!";
    //    TODO: Save log (serial output) -> filename.log
    if (QString(data).contains("Done", Qt::CaseInsensitive)){
        ui->actionStart->setEnabled(true);
        ui->actionPlotSignals->setEnabled(true);
        showStatusMessage(tr("Data saved successfully!"), StatusFlag::Saved);
        ui->serialMonitorTextEdit->insertPlainText("\n--------------------------\n");
        // Close
        qDebug() << data;
    }
}

void MainWindow::plotSignals()
{
    ui->actionPlotSignals->setEnabled(false);
    qDebug() << "Plot signals!";
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
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveDataCollection()));
    connect(ui->actionPlotSignals, SIGNAL(triggered()), this, SLOT(plotSignals()));
    //connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::showStatusMessage(const QString &message, qint8 sF)
{
    status->setText(message);
    statusFlag = sF;
}

void MainWindow::changeIconStatus(qint8 iStatus = 0)
{
    // red
    if (iStatus == 1)
        iconStatus->setPixmap(*pixmapS1);
    // yellow
    else if (iStatus == 2)
        iconStatus->setPixmap(*pixmapS2);
    // green
    else if (iStatus == 3)
        iconStatus->setPixmap(*pixmapS3);
    // blank
    else
        iconStatus->setPixmap(*pixmapS0);
}
