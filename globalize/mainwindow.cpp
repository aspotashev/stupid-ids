#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QVBoxLayout *layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(layout);

    for (int i = 0; i < 100; i ++)
        layout->addWidget(new QPushButton("123"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
