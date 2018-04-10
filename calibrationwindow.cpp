#include "calibrationwindow.h"
#include "ui_calibrationwindow.h"

CalibrationWindow::CalibrationWindow(QWidget *parent) :
    QCalibrationWindow(parent),
    ui(new Ui::CalibrationWindow)
{
    ui->setupUi(this);

}

CalibrationWindow::~CalibrationWindow()
{

    delete ui;
}


