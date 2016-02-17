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
    void writeData(const QByteArray &data);
    void readData();
    void initialFirmwareSetup();
    void startDataCollection();
    void stopDataCollection();

    void handleError(QSerialPort::SerialPortError error);

private:
    void initActionsConnections();
    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;
    QLabel *status;
    SettingsDialog *settings;
    TrialSetup *trialSetup;
    Patient *patient;
    QSerialPort *serial;
};

#endif // MAINWINDOW_H
