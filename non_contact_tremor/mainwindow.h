#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "plot.h"

#include <QFile>
#include <QMainWindow>
#include <QSerialPort>
#include <QTextStream>

class QLabel;

namespace Ui {
class MainWindow;
}

class SettingsDialog;
class TrialSetup;
class Patient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum StatusFlag {
        Init = 0,
        Connected = 1,
        Disconnected = 2,
        Recording = 3,
        Stopped = 4,
        ConversionDone = 5,
        LogSaving = 6,
        Saving = 7,
        Saved = 8,
        Plotting = 9,
        Plotted = 10,
        Unknown = -1
    };
    Q_ENUM(StatusFlag)

//    void saveDataHandler(char *data);

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void about();
    void readData();
    qint64 writeData(const QByteArray &data);
    qint64 writeData();
    void initialFirmwareSetup();
    void startDataCollection();
    void stopDataCollection();
    void saveDataCollection();
    void plotSignals();
    void plotHandler();
    void handleError(QSerialPort::SerialPortError error);

private:
    void initActionsConnections();
    void showStatusMessage(const QString &message, qint8 sF);
    void changeIconStatus(qint8 iStatus);
    void stopDataHandler(char *data);
    void saveDataHandler(char *data);
    void saveDataFinished();
    void appendTextSerialMonitor(const QString &text);

    Ui::MainWindow *ui;
    QLabel *status;
    QLabel *iconStatus;
    QPixmap *pixmapS0;
    QPixmap *pixmapS1;
    QPixmap *pixmapS2;
    QPixmap *pixmapS3;
    SettingsDialog *settings;
    TrialSetup *trialSetup;
    Patient *patient;
    QSerialPort *serial;
    QByteArray *_ba;    
    qint8 statusFlag;
    QString fullFileName;
    QFile *csvFile;
    QTextStream *stream;
    QString serialMonitorText;
    Plot *plot;
};

#endif // MAINWINDOW_H
