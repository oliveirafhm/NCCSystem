#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

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

    void handleError(QSerialPort::SerialPortError error);

private:
    void initActionsConnections();
    void showStatusMessage(const QString &message);
    void changeIconStatus(qint8 iStatus);

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
};

#endif // MAINWINDOW_H
