#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qlabel.h>
#include "imagecreator.h"
#include <QThread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void imageShowCalibrate(QVector<QRgb>);
    void imageShowChart(int,int,int);
    void imageShowBrain(int *ledSendings);
    void calibrationPercentage(double);
    void calibrationSuccess();

private slots:
    void on_pushButton_clicked();
    void processResults();
    void errorSerialQuit();
    void on_comboBox_currentIndexChanged(int index);

signals:
    void launchNucleoChart();
    void launchNucleoCalibrate();
    void launchNucleoBrain();
    void launchNucleoCalibrateADCs();
    void changeMode(int);

private:
    Ui::MainWindow *ui;
    ImageCreator* imgCreator;
    QThread* workerThread;
    bool* createLoop;
    QImage* originalBrain;
    int mode;
    QImage* oldImage;
    QVector<int>* imagePixelsLeft;
    QVector<int>* imagePixelsRight;
    QVector<int>* imagePixelsTotal;
    int chartSize;

};

#endif // MAINWINDOW_H
