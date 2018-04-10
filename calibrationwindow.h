#ifndef CALIBRATIONWINDOW_H
#define CALIBRATIONWINDOW_H

#include <QMainWindow>
#include <qlabel.h>
#include "imagecreator.h"
#include <QThread>

namespace Ui {
class CalibrationWindow;
}

class CalibrationWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit CalibrationWindow(QWidget *parent = 0);
    ~CalibrationWindow();

public slots:

private slots:

signals:

private:
    Ui::CalibrationWindow *ui;
};

#endif // CALIBRATIONWINDOW_H
