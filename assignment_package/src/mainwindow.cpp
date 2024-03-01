#include "mainwindow.h"
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), cHelp()
{
    ui->setupUi(this);
    playerInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width() * 0.75, 0));
//    loadingPopup.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(0, this->height() * 0.5));
    ui->loadingHolder->hide();

    connect(ui->mygl, SIGNAL(sig_sendPlayerPos(QString)), &playerInfoWindow, SLOT(slot_setPosText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerVel(QString)), &playerInfoWindow, SLOT(slot_setVelText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerAcc(QString)), &playerInfoWindow, SLOT(slot_setAccText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerLook(QString)), &playerInfoWindow, SLOT(slot_setLookText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerChunk(QString)), &playerInfoWindow, SLOT(slot_setChunkText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerTerrainZone(QString)), &playerInfoWindow, SLOT(slot_setZoneText(QString)));

    connect(ui->mygl,SIGNAL(sig_toggleProgress()),this,SLOT(on_toggleProgress()));
    connect(ui->mygl,SIGNAL(sig_quit()),this,SLOT(on_actionQuit_triggered()));
    connect(ui->mygl,SIGNAL(sig_sendFPS(float)),this,SLOT(on_getFPS(float)));
    connect(ui->mygl,SIGNAL(sig_sendPlayerPos(QString)),this,SLOT(on_getPOS(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionQuit_triggered()
{
    ui->mygl->save();
    ui->loadingLabel->setText("Saving...");
    ui->loadingHolder->show();
    //loadingPopup.setText("Saving...");
    //loadingPopup.show();

}

void MainWindow::on_actionCamera_Controls_triggered()
{
    cHelp.show();
}

void MainWindow::initalize(QString savename) {
    this->savename = savename;
    ui->loadingLabel->setText("Loading...");
    //loadingPopup.setText("Loading...");
    playerInfoWindow.show();

    ui->mygl->load(savename);
    ui->mygl->setFocus();
    this->show();
    //loadingPopup.show();
    ui->loadingHolder->show();
}

void MainWindow::on_toggleProgress() {
//    if (loadingPopup.isHidden()) {
//        loadingPopup.show();
//    } else {
//        loadingPopup.hide();
//        ui->mygl->setFocus();
//    }
    if (ui->loadingHolder->isHidden()) {
        ui->loadingHolder->show();
    } else {
        ui->loadingHolder->hide();
        ui->mygl->setFocus();
    }
}

void MainWindow::on_getFPS(float dt) {
    ui->fpsLabel->setNum(1.f / dt);
}

void MainWindow::on_getPOS(QString str) {
    ui->posLabel->setText(str);
}
