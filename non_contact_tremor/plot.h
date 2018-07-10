#ifndef PLOT_H
#define PLOT_H

#include <QFile>
#include <QTextStream>
#include <QVector>

#include <QDebug>
#include <QObject>

class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(QObject *parent = nullptr);
    ~Plot();

    bool loadData(const QString &path);

    // First and second chart data (x- and y-axis of PS25454)
    QVector<double> x, y1, y2;
    // Additional data
    QVector<double> batteryStatus, analogInA, digitalInA, digitalInB;
signals:

public slots:

private:
    void parseData(QTextStream &data);
    double adcConversion(qint32 value, float systemVoltage , qint32 factor);
    void clearData();

    QFile *csvFile;
    QTextStream *stream;
};

#endif // PLOT_H
