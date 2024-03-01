#ifndef STARTWINDOW_H
#define STARTWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFile>
#include "mainwindow.h"

namespace Ui {
class StartWindow;
}

class StartWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StartWindow(QWidget *parent, MainWindow* game);
    ~StartWindow();

private:
    Ui::StartWindow *ui;
    MainWindow* game;

    void populateSaves();
    void makeNewGameWindow(QString savename);

private slots:
    void start_load_game();
    void start_new_game();
};

#endif // STARTWINDOW_H
