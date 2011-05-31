#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "messageeditorwidget.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    MessageEditorWidget *addMessageEditor(MessageGroup *messageGroup);

private:
    Ui::MainWindow *ui;
    QVBoxLayout *m_layout;
};

#endif // MAINWINDOW_H
