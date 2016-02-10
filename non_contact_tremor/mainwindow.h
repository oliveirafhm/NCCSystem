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

    void handleError(QSerialPort::SerialPortError error);

private:
    void initActionsConnections();
    void showStatusMessage(const QString &message);

    Ui::MainWindow *ui;
    QLabel *status;
    SettingsDialog *settings;
    TrialSetup *trialSetup;
    QSerialPort *serial;
};

#endif // MAINWINDOW_H
