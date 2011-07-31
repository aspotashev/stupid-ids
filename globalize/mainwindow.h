#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

#include "messageeditorwidget.h"

namespace Ui {
    class MainWindow;
}

class QVBoxLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    MessageEditorWidget *addMessageEditor(MessageGroup *messageGroup);
    void loadConflicting();

private:
    Ui::MainWindow *ui;
    QVBoxLayout *m_layout;
};

#endif // MAINWINDOW_H
