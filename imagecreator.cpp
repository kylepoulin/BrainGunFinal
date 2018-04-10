#include "imagecreator.h"
#include "wiringPi.h"
#include "wiringSerial.h"
#include <QDebug>
#include <QThread>
#include <math.h>
#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <deque>


ImageCreator::ImageCreator(QObject *parent) : QObject(parent)
{
    wiringPiSetup();
    createLoop=0;
    pinMode(8,OUTPUT);
    digitalWrite(8,0);
    std::ifstream source;
    source.open(":/ledScales.txt", std::fstream::in);
    if(!source){
        qWarning("No data file opened");
        emit errorQuit();
    } else {
        for(int i=0;i<16;i++){
            source >> ledScales[i];
        }
    }
    source.close();
}

void ImageCreator::calibrate()
{
    //Open the Serial Connection
    int fd = serialOpen("/dev/ttyAMA0", 19200);
    if(fd == -1){
        //serialClose(fd);
        qWarning("No worko, error: %s\n", strerror(errno));
        emit errorQuit();
    }
    //while loop to check for new bytes
    int mapIt = 0;
    char led[2];
    char mean[2];
    char std[2];
    bool reading = 0;
    int calibrationSize = 10000;
    uint8_t ledOuts[16];
    uint8_t ledArray[16][calibrationSize];
    QVector<QRgb> vector(16);
    //Set the Nucleo to start reading data points and sending over serial
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
    serialFlush(fd);

    for(int j=0; j<calibrationSize; j++)
    {
        for(int i=0; i<16; i++){
            while(true&&createLoop){
                QThread::usleep(100);
                if(serialDataAvail(fd)){
                    //Read LED number, aka decide which vector to put into it
                    led[0]= (uint8_t)serialGetchar(fd);
                    //qWarning("LED reading: %d", led[0]);
                    reading=1;
                    break;
                }
            }
            //qWarning("made it to second reading");
            while(true&&createLoop){
                QThread::usleep(100);
               // qDebug() << "We're attempting to read2";
                if(serialDataAvail(fd)){
                    mean[0] = (uint8_t)serialGetchar(fd);
                    //qWarning("LED mean: %d", mean[0]);
                    break;
                }
            }
            while(true&&createLoop){
                QThread::usleep(100);
                if(serialDataAvail(fd)){
                    std[0] = (uint8_t)serialGetchar(fd);
                    reading=1;
                    break;
                }
            }
            if(!createLoop){
                break;
            }
            if(reading){

                if((uint8_t)i == (uint8_t)led[0]){
                    qWarning("got to ledOuts array");
                    ledOuts[i] = (uint8_t)mean[0];
                    qWarning("got to ledArray array");
                    ledArray[i][j] = (uint8_t)mean[0];
                    reading = 0;
                } else {
                    qWarning("Mismatched %d led reading with %d loop index\n",i,led[0]);
                    emit errorQuit();
                }
            }
            if(mapIt==15){
                //output pixels and the calibration completion
                for(int i=0; i<16; i++){
                    qWarning("made it to vector qrgb vec");
                    vector[i] = qRgb(ledOuts[i],0,0);
                }
                qWarning("made it to vector emission");
                emit sendImgCalibrate(vector);

                //emit the percentage finished
                //emit calibrationCompletionPercentage(j/100000);
                mapIt=0;
            }
            mapIt++;
        }
    }
    //Set LED Scales: Get Mean of all LEDs over this time
    //TO-DO -> Scale LED outputs with their calibrated values
    if(createLoop){
        for(int i=0;i<16;i++){
            for(int j=0;j<calibrationSize;j++){
                ledOuts[i] += ledArray[i][j];
            }
            ledScales[i] = ledOuts[i] / calibrationSize;
        }
        //set Led values in the calibration resource file
        std::ofstream source(":/ledScales.txt");
        if(source.is_open()){
            source << ledScales[0] << " " << ledScales[1] << " " << ledScales[2] << " " << ledScales[3] << " " <<
                      ledScales[4] << " " << ledScales[5] << " " << ledScales[6] << " " << ledScales[7] << " " <<
                      ledScales[8] << " " << ledScales[9] << " " << ledScales[10] << " " << ledScales[11] << " " <<
                      ledScales[12] << " " << ledScales[13] << " " << ledScales[14] << " " << ledScales[15];
        } else {
            qWarning("Could not write to file");
            emit errorQuit();
        }
        //emit success to the pi
        emit calibrationSuccess();
        createLoop=0;
    }
    //Deactivate the nucleo and close the serial connection
    serialClose(fd);
    serialFlush(fd);
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
}

void ImageCreator::calibrateADCs()
{
    //Open up the serial connection to the Nucleo
    int fd = serialOpen("/dev/ttyAMA0", 19200);
    if(fd == -1){
        //serialClose(fd);
        qWarning("No worko, error: %s\n", strerror(errno));
        emit errorQuit();
    }
    int calibrationPower = 5000;


    double mean[2];
    uint8_t adc[2];
    uint8_t value[2];
    mean[0]=0;
    mean[1]=0;
    double percent_complete=0;
    //qWarning("Made it attempted reading!");
    serialFlush(fd);
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
    for(int i=1; i<calibrationPower+1; i++){
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                //Read ADC number
                adc[0] = (uint8_t)serialGetchar(fd);
                //qWarning("ADC reading: %d", adc);
                break;
            }
        }
        //qWarning("made it to second reading");
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                value[0] = (uint8_t)serialGetchar(fd);
                //qWarning("Value reading: %d", value);
                break;
            }
        }
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                //Read ADC number
                adc[1] = (uint8_t)serialGetchar(fd);
                //qWarning("ADC reading: %d", adc);
                break;
            }
        }
        //qWarning("made it to second reading");
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                value[1] = (uint8_t)serialGetchar(fd);
                //qWarning("Value reading: %d", value);
                break;
            }
        }
        if(!createLoop){
            break;
        }
        qWarning("ADC %d, value %d",(int)adc[0], value[0]);
        qWarning("ADC %d, value %d",(int)adc[1], value[1]);
        mean[adc[0]] += (int)value[0];
        mean[adc[1]] += (int)value[1];

        //qWarning("%d mean 1 and %d mean 2",mean[0],mean[1]);
        if(calibrationPower*1000%i==0){
            //emit completion percentage
            emit calibrationPercentage(i);
        }
    }

    if(createLoop){
        qWarning("Made it to the end");
        mean[0] = mean[0]/(calibrationPower);
        mean[1] = mean[1]/(calibrationPower);
        qWarning("%f    %f",mean[0],mean[1]);
        //set ADC values in the calibration resource file
        std::ofstream source("/home/pi/imageTest/adcCalibrationValues.txt", std::ofstream::out);
        if(source.is_open()){
            source << mean[0] << "  " << mean[1];
        } else {
            qWarning("Could not write to file");
            emit errorQuit();
        }
        emit calibrationSuccess();
        createLoop=0;
    }
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
    serialClose(fd);
    serialFlush(fd);
}


void ImageCreator::doWorkBrain()
{
    int fd = serialOpen("/dev/ttyAMA0", 19200);
    if(fd == -1){
        //serialClose(fd);
        qWarning("No worko, error: %s\n", strerror(errno));
        emit errorQuit();
    }
    int ledReadings[16];
    int ledSendings[8];
    uint8_t led;
    uint8_t mean;
    uint8_t std;
    bool reading = 0;
    int mapIt = 0;
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
    serialFlush(fd);
    while(createLoop){
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                //Read LED number
                led = (uint8_t)serialGetchar(fd);
                reading=1;
                break;
            }
        }
        //qWarning("made it to second reading");
        while(true&&createLoop){
            QThread::usleep(100);
           // qDebug() << "We're attempting to read2";
            if(serialDataAvail(fd)){
                mean = (uint8_t)serialGetchar(fd);
                //qWarning("LED mean: %d", mean[0]);
                break;
            }
        }
        while(true&&createLoop){
            QThread::usleep(100);
            if(serialDataAvail(fd)){
                std = (uint8_t)serialGetchar(fd);
                reading=1;
                break;
            }
        }
        if(!createLoop){
            break;
        }

        if(reading){
            ledReadings[led]=mean;
        }

        if(mapIt==15){
            //output pixels and the calibration completion
            for(int i=0; i<8; i++){
                ledSendings[i] = (int)std::round((ledReadings[2*i]+ledReadings[2*i+1])/2);
            }
            emit sendImgBrain(ledSendings);
            mapIt=0;
        }
        mapIt++;
    }
    serialClose(fd);
    serialFlush(fd);
    digitalWrite(8,1);
    QThread::usleep(500);
    digitalWrite(8,0);
}

void ImageCreator::doWorkChart()
{

    int fd = serialOpen("/dev/ttyAMA0", 19200);
    if(fd == -1){
        //serialClose(fd);
        qWarning("No worko, error: %s\n", strerror(errno));
        emit errorQuit();
    }    
    QVector<int> means(16);
    //while loop to check for new bytes goes here
        int mapIt = 1;
        uint8_t led;
        uint8_t mean;
        uint8_t std;
        bool reading = 0;
        digitalWrite(8,1);
        QThread::usleep(500);
        digitalWrite(8,0);
        serialFlush(fd);

        qWarning("Made it to While loop");
        while(createLoop)
        {
            while(true&&createLoop){
                QThread::usleep(100);
                if(serialDataAvail(fd)){
                    //Read LED number, aka decide which vector to put into it
                    led= (uint8_t)serialGetchar(fd);
                    //qWarning("LED reading: %d", led);
                    reading=1;
                    break;
                }
            }
            while(true&&createLoop){
                QThread::usleep(100);
               // qDebug() << "We're attempting to read2";
                if(serialDataAvail(fd)){
                    means[led] = (uint8_t)serialGetchar(fd);
                    //qWarning("Mean: %d", mean);

                    break;
                }
            }
            while(true&&createLoop){
                QThread::usleep(100);
                if(serialDataAvail(fd)){
                    std = (uint8_t)serialGetchar(fd);
                    reading=1;
                    break;
                }
            }
            if(!createLoop){
                break;
            }
            if(reading){
                //qWarning("made it to reading");
                //slopeVectors.push_back(mean);
                //slopeVectors.pop_front();
            }
            if(mapIt==16){
                //Doing heavy arithmetic in this area of the code causes really slow performance
                //Consider shifting this to the Nucleo
                /*
                for(int i=0; i<16; i++){
                    //double earlySlope=0;
                    int laterSlope=0;

                    for(int j=0; j<(int)std::round(slopeMeanArraySize/5); j++){
                        //earlySlope += slopeVectors[16*j+i];
                        laterSlope += slopeVectors[slopeMeanArraySize*16-1-16*(j+1)+i];
                        //qWarning("SlopeVector value %d to be averaged: %d",j,slopeVectors[slopeMeanArraySize*16-1-16*(j+1)+i]);
                    }
                    //qWarning("Mean before divide: %d", laterSlope);
                    // earlySlope = earlySlope*10/slopeMeanArraySize;
                    laterSlope = (int)std::round(laterSlope*5/slopeMeanArraySize);
                    //qWarning("Mean after divide: %d", laterSlope);

                    means[i] = (int)slopeVectors.at(slopeMeanArraySize*16-17+i);
                    //qWarning("mean: %d\n",means[i]);
                    //means[i] = (int)laterSlope;
                    //means[i] = round((laterSlope-earlySlope)/(slopeVectors.size()/16));
                    //have to verify that the slopes are ~= and opposite to represent
                    //oxygenation difference
                }
                */
                int meanleft=0, meanright=0, meantotal=0;
                meanleft = 255-std::round((means.at(0) + means.at(4) +means.at(8) +means.at(12))/4);
                meanright = 255-std::round((means.at(2) + means.at(6) +means.at(10) +means.at(14))/4);
                meantotal = std::round((meanleft+meanright)/2);
                //qWarning("meanleft: %d", meanleft);
                /*
                imagePixelsLeft.append(meanleft);
                //qWarning("%d",slopeVectors[slopeVectors.size()-1-15]);
                imagePixelsLeft.pop_back();
                imagePixelsRight.append(meanright);
                imagePixelsRight.pop_back();
                imagePixelsTotal.append(meantotal);
                imagePixelsTotal.pop_back();
                */
                emit sendImgChart(meanleft,meanright,meantotal);
                mapIt=0;
            }
            mapIt++;
        }
        serialClose(fd);
        digitalWrite(8,1);
        QThread::usleep(500);
        digitalWrite(8,0);
}



bool* ImageCreator::serveLoopEnder()
{
    bool* loopEnder = &createLoop;
    return loopEnder;
}

void ImageCreator::changeMode(int index)
{
    mode = index;
}
