#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "cameracontrolshelp.h"
#include "playerinfo.h"
#include <QTimer>

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent);
    ~MainWindow();
    void initalize(QString savename);

private slots:
    void on_actionQuit_triggered();

    void on_actionCamera_Controls_triggered();

    void on_toggleProgress();

    void on_getFPS(float);
    void on_getPOS(QString);

private:
    Ui::MainWindow *ui;
    CameraControlsHelp cHelp;
    PlayerInfo playerInfoWindow;
//    LoadingPopup loadingPopup;
    QString savename;
};


#endif // MAINWINDOW_H
