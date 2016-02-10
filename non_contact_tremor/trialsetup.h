#ifndef TRIALSETUP_H
#define TRIALSETUP_H

#include <QDialog>
#include <QStandardPaths>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class TrialSetup;
}

QT_END_NAMESPACE

class TrialSetup : public QDialog
{
    Q_OBJECT

public:
    struct TrialSetupConfig{
        quint8 outputSignal[3] = {0,0,0};
        quint16 sampleRate = 200;
        quint8 collectionTimeOut = 15;
        QString directoryPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    };

    explicit TrialSetup(QWidget *parent = 0);
    ~TrialSetup();

    TrialSetupConfig trialSetupConfig() const;

private slots:
    void on_chooseDirectoryButton_clicked();
    void apply();

private:
    void fillUI();
    void updateTrialSetupConfig();

private:
    Ui::TrialSetup *ui;
    TrialSetupConfig currentTrialSetup;
};

#endif // TRIALSETUP_H
