#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "wavwindow.h"

using std::unique_ptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    wav_window = unique_ptr<WavWindow>(new WavWindow());
    png_window = unique_ptr<PNGWindow>(new PNGWindow());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_openWavButton_clicked()
{
    wav_window->show();
}

void MainWindow::on_openPngButton_clicked() {
    png_window->show();
}

