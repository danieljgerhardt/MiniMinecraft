#include "startwindow.h"
#include "ui_startwindow.h"
#include <QMessageBox>

StartWindow::StartWindow(QWidget *parent, MainWindow* game) :
    QMainWindow(parent),
    ui(new Ui::StartWindow),  game(game)
{
    ui->setupUi(this);
    populateSaves();

    connect(ui->newGameStart,SIGNAL(pressed()),this,SLOT(start_new_game()));
    connect(ui->loadGameStart,SIGNAL(pressed()),this,SLOT(start_load_game()));
}

StartWindow::~StartWindow()
{
    delete ui;
}

void StartWindow::populateSaves() {
    ui->loadGameList->clear();

    QDir directory("../saves");
    directory.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->loadGameList->addItems(directory.entryList());
}

void StartWindow::start_load_game() {
    QListWidgetItem* item = ui->loadGameList->currentItem();
    if (item == nullptr) {
        QMessageBox::critical(this,"cannot load save","please select a save");
    } else {
        makeNewGameWindow(item->text());
    }
}

void StartWindow::start_new_game() {
    QString name = ui->newGameName->text();
    int seed = ui->newGameSeed->value();

    QDir directory("../saves");
    if (!directory.mkdir(name)) {
        QMessageBox::critical(this,"invalid name","this save name is in use");
        return;
    }

    QFile file(directory.path()+"/"+name+"/"+name+".save");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    // player x (float), player y (float), player z (float), seed (int)
    out << (float)48.f << (float)170.f << (float)48.f << (int)seed;

    file.close();

    makeNewGameWindow(name);
}

void StartWindow::makeNewGameWindow(QString savename) {
    game->initalize(savename);
    this->hide();
}
