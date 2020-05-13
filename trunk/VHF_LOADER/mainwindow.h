#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "vhfmonitorthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& exe, QWidget *parent = 0);
    ~MainWindow();
public slots:

private:
    Ui::MainWindow *ui;
    VHFMonitorThread* mThread;

};

#endif // MAINWINDOW_H
