#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <math.h>
#include <QColor>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //Set up the user interface
    ui->setupUi(this);
    ui->label1->adjustSize();
    ui->pushButton->adjustSize();
    //setCentralWidget(ui->label1);

    //Set the image creator thread up
    workerThread = new QThread();
    imgCreator = new ImageCreator();
    imgCreator->moveToThread(workerThread);
    workerThread->start();

    //Customizeable chart size (Affects the time it takes for a newly charted value to travel across the screen)
    chartSize = 500;

    //Cache the default image for update
    oldImage = new QImage(chartSize,255, QImage::Format_RGB32);
    oldImage->fill(qRgb(255,255,255));

    imagePixelsLeft = new QVector<int>(chartSize);
    imagePixelsLeft->fill(255-113);
    imagePixelsRight = new QVector<int>(chartSize);
    imagePixelsRight->fill(255-113);
    imagePixelsTotal = new QVector<int>(chartSize);
    imagePixelsTotal->fill(255-113);

    //Allow for transfer of RGB and int QVectors across threads
    qRegisterMetaType<QVector<QRgb>>("QVector<QRgb>");
    qRegisterMetaType<QVector<int>>("QVector<int>");

    //Connect all signal/slot functions to allow communication between threads
    QObject::connect(workerThread, SIGNAL(finished()), imgCreator, SIGNAL(QObjet::deleteLater()));
    QObject::connect(this, SIGNAL(launchNucleoChart()), imgCreator, SLOT(doWorkChart()));
    QObject::connect(this, SIGNAL(launchNucleoCalibrate()), imgCreator, SLOT(calibrate()));
    QObject::connect(this, SIGNAL(launchNucleoBrain()), imgCreator, SLOT(doWorkBrain()));
    QObject::connect(this, SIGNAL(launchNucleoCalibrateADCs()), imgCreator, SLOT(calibrateADCs()));
    QObject::connect(imgCreator, SIGNAL(resultsReady()), this, SLOT(processResults()));
    QObject::connect(imgCreator, SIGNAL(sendImgCalibrate(QVector<QRgb>)), this, SLOT(imageShowCalibrate(QVector<QRgb>)));
    QObject::connect(imgCreator, SIGNAL(sendImgChart(int,int,int)), this, SLOT(imageShowChart(int,int,int)));
    QObject::connect(imgCreator, SIGNAL(sendImgBrain(int*)), this, SLOT(imageShowBrain(int*)));
    QObject::connect(imgCreator, SIGNAL(calibrationPercentage(double)), this, SLOT(calibrationPercentage(double)));
    QObject::connect(imgCreator, SIGNAL(calibrationSuccess()), this, SLOT(calibrationSuccess()));


    QObject::connect(imgCreator, SIGNAL(errorQuit()), this, SLOT(errorSerialQuit()));
    QObject::connect( this, SIGNAL(changeMode(int)), imgCreator, SLOT(changeMode(int)));

    //Create the "semaphore" which acts to start and stop the active thread
    createLoop = imgCreator->serveLoopEnder();
    *createLoop = 0;
    originalBrain = new QImage();
    originalBrain->load(":/brain.bmp");
    mode = 0;
}

MainWindow::~MainWindow()
{
    //Clean up dynamically allocated resources
    if(workerThread->isRunning()){
        *createLoop = 0;
        workerThread->quit();
        workerThread->wait(100);
    }
    delete workerThread;
    if(originalBrain!=NULL){
        delete originalBrain;
    }
    delete oldImage;
    delete imgCreator;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    //Logic for the Start button depending on the mode of operation
    if(mode==0){
        //Launch the time chart display
        if(*createLoop==0){
            ui->pushButton->setText("Stop");
            *createLoop = 1;
            emit launchNucleoChart();

            QImage image(4,2, QImage::Format_RGB32);
            for(int i=0; i<4; i++){
                image.setPixel(i,0,qRgb(i*20,i*6,0));
                image.setPixel(i,1,qRgb(i*18,i*4,0));
            }
            const int w = ui->label1->width();
            const int h = ui->label1->height();
            ui->label1->setPixmap(QPixmap::fromImage(image).scaled(w,h,Qt::IgnoreAspectRatio));
            ui->label1->show();

            for(int i=0; i<4; i++){
                image.setPixel(i,0,qRgb(0,i*6,i*20));
                image.setPixel(i,1,qRgb(0,i*4,i*18));
            }
            ui->label1->setPixmap(QPixmap::fromImage(image).scaled(w,h,Qt::IgnoreAspectRatio));
            ui->label1->show();
        } else {
            ui->pushButton->setText("Start");
            QImage image(4,2, QImage::Format_RGB32);
            for(int i=0; i<4; i++){
                image.setPixel(i,0,qRgb(i*20,i*2,0));
                image.setPixel(i,1,qRgb(i*18,i*1,0));
            }
            const int w = ui->label1->width();
            const int h = ui->label1->height();
            ui->label1->setPixmap(QPixmap::fromImage(image).scaled(w,h,Qt::IgnoreAspectRatio));
            ui->label1->show();
            *createLoop = 0;
        }
    } else if(mode==1){
        //emit the calibration signal
        //TO-DO: figure out a proper display for the time to finish calibration
            //imageCreator should send a percent finished, as well as the pixel values
            //mainwindow should update the pixel colors in the main window, and also show a new window with a loading bar
        if(*createLoop==0){
            *createLoop = 1;
            ui->pushButton->setText("Stop");
            emit launchNucleoCalibrate();
        } else {
            ui->pushButton->setText("Start");
            *createLoop = 0;
        }
    } else if (mode==2) {
        //Launch the Brain Intensity Map display
        if(*createLoop==0){
            ui->pushButton->setText("Stop");
            *createLoop = 1;
            emit launchNucleoBrain();
            const int w = ui->label1->width();
            const int h = ui->label1->height();
            ui->label1->setPixmap(QPixmap::fromImage(*originalBrain).scaled(w,h,Qt::KeepAspectRatio));
            ui->label1->show();
            qWarning("width: %d", originalBrain->width());
            qWarning("height: %d", originalBrain->height());


        } else {
            ui->pushButton->setText("Start");
            *createLoop = 0;
        }
    } else {
        //Perform a calibration for the ADCs or APDs
        if(*createLoop==0){
            ui->pushButton->setText("Stop");
            *createLoop = 1;
            emit launchNucleoCalibrateADCs();
        } else {
            ui->pushButton->setText("Start");
            *createLoop = 0;
        }
    }
}

void MainWindow::imageShowCalibrate(QVector<QRgb> pixels)
{
    //Shows the average reading for each LED pulse in time
    QImage image(8,2,QImage::Format_RGB32);
    for(int i=0; i<8; i++){
        image.setPixel(i,0,pixels[i]);
        image.setPixel(i,1,pixels[i+8]);
    }
    const int w = ui->label1->width();
    const int h = ui->label1->height();
    ui->label1->setPixmap(QPixmap::fromImage(image).scaled(w,h,Qt::IgnoreAspectRatio));
    ui->label1->show();
}

void MainWindow::imageShowBrain(int* ledSendings){
    QImage image(*originalBrain);
    //left side.
    //Display the intensities at each quadrant of the left optode
    //Red in the white areas of the image should increase, while blue and green in the dark areas should decrease
    for(int i=std::round(image.width()/3-120); i<std::round(image.width()/3); i++){
        for(int j=std::round(image.height()/2); j>std::round(image.height()/2-120); j--){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[0],255-ledSendings[0]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[0],20,20));
            }
        }
    }
    for(int i=std::round(image.width()/3-120); i<std::round(image.width()/3); i++){
        for(int j=std::round(image.height()/2+1); j<std::round(image.height()/2+120); j++){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[2],255-ledSendings[2]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[2],20,20));
            }
        }
    }
    for(int i=std::round(image.width()/3); i<std::round(image.width()/3+120); i++){
        for(int j=std::round(image.height()/2); j>std::round(image.height()/2-120); j--){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[4],255-ledSendings[4]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[4],20,20));
            }
        }
    }
    for(int i=std::round(image.width()/3); i<std::round(image.width()/3+120); i++){
        for(int j=std::round(image.height()/2+1); j<std::round(image.height()/2+120); j++){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[6],255-ledSendings[6]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[6],20,20));
            }

        }
    }
    //right side
    for(int i=std::round(image.width()*2/3-120); i<std::round(image.width()*2/3); i++){
        for(int j=std::round(image.height()/2); j>std::round(image.height()/2-120); j--){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[1],255-ledSendings[1]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[1],20,20));
            }

        }
    }
    for(int i=std::round(image.width()*2/3-120); i<std::round(image.width()*2/3); i++){
        for(int j=std::round(image.height()/2+1); j<std::round(image.height()/2+120); j++){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[3],255-ledSendings[3]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[3],20,20));
            }
        }
    }
    for(int i=std::round(image.width()*2/3); i<std::round(image.width()*2/3+120); i++){
        for(int j=std::round(image.height()/2); j>std::round(image.height()/2-120); j--){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[5],255-ledSendings[5]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[5],20,20));
            }
        }
    }
    for(int i=std::round(image.width()*2/3); i<std::round(image.width()*2/3+120); i++){
        for(int j=std::round(image.height()/2+1); j<std::round(image.height()/2+120); j++){
            if(image.pixelColor(i,j).red()>113){
                image.setPixel(i,j,qRgb(255,255-ledSendings[7],255-ledSendings[7]));
            } else {
                image.setPixel(i,j,qRgb(ledSendings[7],0,0));
            }
        }
    }
    const int w = ui->label1->width();
    const int h = ui->label1->height();
    ui->label1->setPixmap(QPixmap::fromImage(image).scaled(w,h,Qt::KeepAspectRatio));
    ui->label1->show();
}

void MainWindow::calibrationSuccess()
{
    ui->pushButton->setText("Start (Calibration Succeeded)");
}

void MainWindow::imageShowChart(int left, int right, int total)
{
    //0 4 8 12 are 860 for left, 2 6 10 14 are 860 for right
    //instead: use old image, update pixels
    //Update the pixels with the average of left, right, and both
    for(int i=0;i<chartSize;i++){
        oldImage->setPixel(i,imagePixelsLeft->at(i), qRgb(255,255,255));
        oldImage->setPixel(i,imagePixelsRight->at(i), qRgb(255,255,255));
        oldImage->setPixel(i,imagePixelsTotal->at(i), qRgb(255,255,255));
    }

    imagePixelsLeft->pop_front();
    imagePixelsLeft->append(left);
    imagePixelsRight->pop_front();
    imagePixelsRight->append(right);
    imagePixelsTotal->pop_front();
    imagePixelsTotal->append(total);

    for(int i=0;i<chartSize;i++){
        oldImage->setPixel(i,imagePixelsLeft->at(i), qRgb(200,0,0));
        oldImage->setPixel(i,imagePixelsRight->at(i), qRgb(0,200,0));
        oldImage->setPixel(i,imagePixelsTotal->at(i), qRgb(0,0,200));
    }
    int w = ui->label1->width();
    int h = ui->label1->height();
    ui->label1->setPixmap(QPixmap::fromImage(*oldImage).scaled(w,h,Qt::IgnoreAspectRatio));
    ui->label1->show();
}


void MainWindow::calibrationPercentage(double percentage)
{
    //update window with percentage
    //qWarning("made it to set text with %d", percentage);
    ui->pushButton->setText(QString::number(percentage)+"/5000 Complete (Click to Stop)");
}

void MainWindow::errorSerialQuit()
{
    close();
}

void MainWindow::processResults()
{

}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    //UI interface from the drop down menu
    if(index==0){
        qWarning("Time Chart");
        emit changeMode(index);
        mode = index;
    } else if (index == 1){
        qWarning("Calibrate");
        emit changeMode(index);
        mode = index;
    } else if (index == 2) {
        qWarning("Brain Display");
        emit changeMode(index);
        mode = index;
    } else {
        qWarning("Calibrate ADCs");
        emit changeMode(index);
        mode = index;
    }
}
