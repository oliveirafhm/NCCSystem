#include "plot.h"

Plot::Plot(QObject *parent) : QObject(parent)
{

}

Plot::~Plot()
{

}

void Plot::loadData(const QString &path)
{
      csvFile = new QFile(path);
      if (!csvFile->open(QFile::ReadOnly | QFile::Text))
          return;
      stream = new QTextStream(csvFile);
      parseData(*stream);
}

void Plot::parseData(QTextStream &data)
{
//    QVector<
    while(!data.atEnd()){
        QString line = data.readLine();
//        qDebug() << line;
        QStringList strList = line.split(',');
        // Filter out header
        if(strList[0].toInt() == 0)
            continue;
//        qDebug() << strList;
        //

    }
}
