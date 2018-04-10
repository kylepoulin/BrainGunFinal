#ifndef IMAGECREATOR_H
#define IMAGECREATOR_H

#include <QObject>
#include <QColor>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>

class ImageCreator : public QObject
{
    Q_OBJECT
public:
    explicit ImageCreator(QObject *parent = 0);
    bool* serveLoopEnder();


signals:
    void sendImgCalibrate(QVector<QRgb>);
    void sendImgChart(int,int,int);
    void sendImgBrain(int*);
    void calibrationPercentage(double);
    void calibrationSuccess();
    void resultsReady();
    void errorQuit();

public slots:
    void doWorkChart();
    void doWorkBrain();
    void calibrate();
    void calibrateADCs();
    void changeMode(int index=0);

private:
    bool createLoop;
    int mode;
    float ledScales[16];
};

#endif // IMAGECREATOR_H
