#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "messageeditorwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QVBoxLayout *layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(layout);

    for (int i = 0; i < 100; i ++)
        layout->addWidget(new MessageEditorWidget(ui->scrollAreaWidgetContents));
}

MainWindow::~MainWindow()
{
    delete ui;
}
