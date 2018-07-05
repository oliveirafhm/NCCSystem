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

    void loadData(const QString &path);

signals:

public slots:

private:
    void parseData(QTextStream &data);

//    Ui::MainWindow *gui;
    QFile *csvFile;
    QTextStream *stream;
    QVector<double> x1, y1;
    QVector<double> x2, y2;
};

#endif // PLOT_H
