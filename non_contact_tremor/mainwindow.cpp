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
#include <QMediaPlayer>

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
    ui->actionPlotSignals->setEnabled(true);
    ui->actionPlayBeeps->setEnabled(false);

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
    // TODO: Check if i need to delete something more
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
        // DTR signal (flag to enable serial communication work on win + nativeport)
        // Ref.: https://stackoverflow.com/questions/23198241/unable-to-communicate-with-arduino-using-qserialport-if-the-arduino-ide-has-not#
        serial->setDataTerminalReady(true);
        //
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
    // Reset arduino due
    QByteArray ba = "n";
    writeData(ba);
    //
    if (serial->isOpen())
        serial->close();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    ui->actionStart->setEnabled(false);
    ui->actionStop->setEnabled(false);
//    ui->actionPlotSignals->setEnabled(false);
    ui->actionTrialSetup->setEnabled(true);
    changeIconStatus(1);
    ui->serialMonitorTextEdit->clear();
    serialMonitorText.clear();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Non contact sensor data logger"),
                       tr("Non contact sensor data logger is a software to record "
                          "and plot signals from non-contact capacitive sensors (PS25454). "
                          "There is a specific firmware to use along with this software. "
                          "Check it with the author: Fábio Henrique (oliveirafhm@gmail.com). "
                          "All icons are from www.flaticon.com"));
}

qint64 MainWindow::writeData(const QByteArray &data)
{
    qint64 w = serial->write(data);
    serial->flush();
    return w;
}

qint64 MainWindow::writeData()
{
    qint64 w = serial->write(*_ba);
    serial->flush();
    _ba->clear();
    return w;
}

void MainWindow::readData()
{    
    while(serial->canReadLine()){
        QByteArray data = serial->readLine();
        if (statusFlag != StatusFlag::Saving){
            appendTextSerialMonitor(data.data());
            if (statusFlag == StatusFlag::Stopped){
                stopDataHandler(data.data());
            }
        } else if (statusFlag == StatusFlag::Saving){
            saveDataHandler(data.data());
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
    QTimer::singleShot(1450,this,SLOT(posStart()));
    //
    ui->actionSave->setEnabled(false);
    ui->actionStart->setEnabled(false);
    ui->actionDisconnect->setEnabled(false);
    ui->actionPlotSignals->setEnabled(false);
    // Clear graph ui component
    ui->xAxisPlot->clearGraphs();
    ui->yAxisPlot->clearGraphs();
    ui->xAxisPlot->replot();
    ui->yAxisPlot->replot();
}

void MainWindow::posStart()
{
    ui->actionStop->setEnabled(true);
    ui->actionPlayBeeps->setEnabled(true);
    changeIconStatus(3);
}

void MainWindow::playBeeps()
{
    showStatusMessage(tr("Playing beeps..."), StatusFlag::Playing);
    ui->actionPlayBeeps->setEnabled(false);
    // Play..
    // How to solve media player bug on Windows (release):
    // https://forum.qt.io/topic/28620/solved-qtmultimedia-defaultserviceprovider-requestservice-no-service-found-for-org-qt-project-qt-mediaplayer/9
    player = new QMediaPlayer;
    player->setMedia(QUrl("qrc:/audio/experiment_beeps.mp3"));
    player->setVolume(100);
    player->play();
}

void MainWindow::stopDataCollection()
{
    // Stop audio beeps
    if (statusFlag == StatusFlag::Playing){
        //    ui->actionPlayBeeps->setEnabled(false);
        player->stop();
        // Destroy the pointer
        delete player;
        player = NULL;
    }
    showStatusMessage(tr("Stopped (wait while the file conversion occurs)."), StatusFlag::Stopped);
    QByteArray ba = "2";
    writeData(ba);
    ui->actionStop->setEnabled(false);
    changeIconStatus(2);
}

void MainWindow::stopDataHandler(char *data){
    if (QString(data).contains("Done:", Qt::CaseInsensitive)){
        ui->actionSave->setEnabled(true);
        ui->actionDisconnect->setEnabled(true);
        ui->actionStart->setEnabled(true);
        showStatusMessage(tr("CSV file successfully created in SD Card!"), StatusFlag::ConversionDone);
    }
}

void MainWindow::saveDataCollection()
{    
    TrialSetup::TrialSetupConfig ts = trialSetup->trialSetupConfig();
    if (!ts.justLog){
        showStatusMessage(tr("Saving... (wait while the file is saved)."), StatusFlag::Saving);
        QByteArray ba = "d";
        writeData(ba);
    } else
        showStatusMessage(tr("Log saving..."), StatusFlag::LogSaving);

    ui->actionSave->setEnabled(false);
    ui->actionDisconnect->setEnabled(false);
    ui->actionStart->setEnabled(false);
    // Create file and open it to write    
    fullFileName = ts.directoryPath + QDir::separator() + ts.fileName + ".csv";
    // Rename file name if need it to avoid override
    qint8 nFile = -1;
    while (QFile::exists(fullFileName)){
        nFile ++;
        fullFileName = ts.directoryPath + QDir::separator() + ts.fileName + QString::number(nFile) + ".csv";
    }
    // Date and time | Plesse csv file name | ADC resolution
    serialMonitorText = ui->serialMonitorTextEdit->toPlainText();
    // Get sample count from terminal output to calc file saving percentage
    qint16 sampleCountIndex;
    if (serialMonitorText.count("Sample count:") > 1)
        sampleCountIndex = serialMonitorText.lastIndexOf("Sample count:");
    else
        sampleCountIndex = serialMonitorText.indexOf("Sample count:");
    sampleCountIndex += QString("Sample count:").length()+1;
    QString s = serialMonitorText.at(sampleCountIndex);
    QString sampleCountString;
    while(s != "\n"){
        sampleCountString += s;
        sampleCountIndex++;
        s = serialMonitorText.at(sampleCountIndex);
    }
    sampleCount = sampleCountString.toInt();
//    qDebug() << sampleCountString;
    // Create header file
    qint16 csvNameIndex;
    if (serialMonitorText.count(".csv") > 1)
        csvNameIndex = serialMonitorText.lastIndexOf(".csv");
    else
        csvNameIndex = serialMonitorText.indexOf(".csv");

    QString psFileName = serialMonitorText.mid(csvNameIndex - 6,10);

    if (!ts.justLog){
        csvFile = new QFile(fullFileName);
        if (csvFile->open(QFile::WriteOnly | QFile::Text)){
            stream = new QTextStream(csvFile);
            *stream << QDateTime::currentDateTime().toString() << " |  "
                    << psFileName << " | " << "ADC: 12 bits & 3.3 volts";
        } else
            showStatusMessage(tr("CSV file opening error! Check file path and filename."), StatusFlag::Unknown);
//        QTimer::singleShot(1000*30,this,SLOT(updateSaveFeedback()));
    }

    // Save log (serial output) -> filename+plessfilename.log
    QFile log(fullFileName.mid(0,fullFileName.length()-4) + "_" +
              psFileName.mid(0,psFileName.length()-4) + ".log");
    if (log.open(QFile::WriteOnly | QFile::Text)){
        QTextStream out(&log);
        out << serialMonitorText;
        log.flush();
        log.close();
    } else
        showStatusMessage(tr("Log file opening error! Check file path and filename."), StatusFlag::Unknown);

    if(ts.justLog)
        saveDataFinished();
}

void MainWindow::saveDataHandler(char *data)
{    
    if (statusFlag == StatusFlag::Saving){
        if (!QString(data).contains("Done", Qt::CaseInsensitive) && !QString(data).contains("Type", Qt::CaseInsensitive)){
            *stream << data;
            // Var used to calc percentage of file saving
            nSampleCount++;
            // Each 36000 samples updates percentage ui feedback
            if (nSampleCount % 26000 == 0){
                float p = nSampleCount / float(sampleCount) * 100.0;
                showStatusMessage("Saving... ("+QString::number(p, 'f', 2)+"%).", StatusFlag::Saving);
            }
        } else if (QString(data).contains("Done", Qt::CaseInsensitive)) {            
            // Flush and close file
            csvFile->flush();
            csvFile->close();
            // Destroy the pointer
            delete csvFile;
            csvFile = NULL;
            delete stream;
            stream = NULL;
            //
            saveDataFinished();
        }
    }
}

void MainWindow::saveDataFinished()
{
    ui->actionStart->setEnabled(true);
    ui->actionDisconnect->setEnabled(true);
    if (statusFlag == StatusFlag::Saving){
        ui->actionPlotSignals->setEnabled(true);
        nSampleCount = 0;
        showStatusMessage(tr("Data saved successfully!"), StatusFlag::Saved);
    } else
        showStatusMessage(tr("Log saved successfully!"), StatusFlag::LogSaved);
    appendTextSerialMonitor("\n--------------------------------\n");
    serialMonitorText.clear();
}

void MainWindow::appendTextSerialMonitor(const QString &text)
{
    ui->serialMonitorTextEdit->moveCursor(QTextCursor::End);
    ui->serialMonitorTextEdit->insertPlainText(text);
    ui->serialMonitorTextEdit->moveCursor(QTextCursor::End);
}

void MainWindow::plotSignals()
{
    // Load external file (option to plot past signals)
    if (statusFlag != StatusFlag::Saved){
        fullFileName = QFileDialog::getOpenFileName(this,
                        tr("Open CSV file"), "/", tr("CSV Files (*.csv)"));
    }
    showStatusMessage(tr("Plotting..."), StatusFlag::Plotting);
//    ui->actionPlotSignals->setEnabled(false);
    QTimer::singleShot(100,this,SLOT(plotHandler()));
}
void MainWindow::plotHandler()
{
    // Clear graph ui component
    ui->xAxisPlot->clearGraphs();
    ui->yAxisPlot->clearGraphs();
    ui->xAxisPlot->replot();
    ui->yAxisPlot->replot();
    // New plot
    plot = new Plot;
    bool loadStatus = plot->loadData(fullFileName);
    if (loadStatus){
        // Create graph and assign data to it
        ui->xAxisPlot->addGraph();
        ui->xAxisPlot->graph(0)->setData(plot->x, plot->y1, true);
        ui->xAxisPlot->graph(0)->setName("Sensor data (x-axis)");
        ui->yAxisPlot->addGraph();
        ui->yAxisPlot->graph(0)->setData(plot->x, plot->y2, true);
        ui->yAxisPlot->graph(0)->setName("Sensor data (y-axis)");
        // Add Digital pulse A to the graphs
        ui->xAxisPlot->addGraph();
        ui->xAxisPlot->graph(1)->setPen(QPen(Qt::red));
        ui->xAxisPlot->graph(1)->setData(plot->x, plot->digitalInA, true);
        ui->xAxisPlot->graph(1)->setName("Digital input A");
        ui->yAxisPlot->addGraph();
        ui->yAxisPlot->graph(1)->setPen(QPen(Qt::red));
        ui->yAxisPlot->graph(1)->setData(plot->x, plot->digitalInA, true);
        ui->yAxisPlot->graph(1)->setName("Digital input A");
        // Set axes labels
        ui->xAxisPlot->xAxis->setLabel("Time (s)");
        ui->xAxisPlot->yAxis->setLabel("V");
        ui->yAxisPlot->xAxis->setLabel("Time (s)");
        ui->yAxisPlot->yAxis->setLabel("V");
        // Set axes ranges
        ui->xAxisPlot->xAxis->setRange(0, plot->x[plot->x.length()-1]);
        ui->xAxisPlot->yAxis->setRange(0, 3.3);
        ui->yAxisPlot->xAxis->setRange(0, plot->x[plot->x.length()-1]);
        ui->yAxisPlot->yAxis->setRange(0, 3.3);

        ui->xAxisPlot->legend->setVisible(true);
        ui->yAxisPlot->legend->setVisible(true);

        // Allow user to drag axis with mouse, zoom with mouse wheel and select graphs by clicking
        ui->xAxisPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        ui->yAxisPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        // Update UI component
//        ui->xAxisPlot->rescaleAxes();
//        ui->yAxisPlot->rescaleAxes();
        ui->xAxisPlot->replot();
        ui->yAxisPlot->replot();
//        ui->xAxisPlot->update();
//        ui->yAxisPlot->update();

        showStatusMessage(tr("Signals already plotted."), StatusFlag::Plotted);
    }
    // Destroy the pointer
    delete plot;
    plot = NULL;
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
    connect(ui->actionPlayBeeps, SIGNAL(triggered()), this, SLOT(playBeeps()));
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
