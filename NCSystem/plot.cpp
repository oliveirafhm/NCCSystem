#include "plot.h"
#include <QtMath>

Plot::Plot(QObject *parent) : QObject(parent)
{

}

Plot::~Plot()
{

}

bool Plot::loadData(const QString &path)
{
      csvFile = new QFile(path);
      if (!csvFile->open(QFile::ReadOnly | QFile::Text))
          return false;
      stream = new QTextStream(csvFile);
      parseData(*stream);
      return true;
}

void Plot::parseData(QTextStream &data)
{
    clearData();
    qint8 adcResolution = 12;
    float mcVoltage = 3.3;
    qint32 factor = qPow(2, adcResolution);
    double initTime = -1;

    while(!data.atEnd()){
        QString line = data.readLine();
        QStringList strList = line.split(',');
        // Filter out header and overruns
        if (strList[0].toInt() == 0)
            continue;
        //
        if (initTime == -1)
            initTime = strList[0].toInt() / 1000000.0;
        x.append(strList[0].toInt() / 1000000.0 - initTime);
        y1.append(strList[2].toInt() * mcVoltage / factor);
        y2.append(strList[3].toInt() * mcVoltage / factor);

        batteryStatus.append(strList[1].toInt() * mcVoltage / factor);
        analogInA.append(strList[4].toInt() * mcVoltage / factor);
        digitalInA.append(strList[5].toInt());
        digitalInB.append(strList[6].toInt());
    }
}

void Plot::clearData()
{
    x.clear();
    y1.clear();
    y2.clear();
    batteryStatus.clear();
    analogInA.clear();
    digitalInA.clear();
    digitalInB.clear();
}

double Plot::adcConversion(qint32 value, float systemVoltage , qint32 factor)
{
    return (value * systemVoltage) / factor;
}
