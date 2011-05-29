#ifndef MESSAGEEDITORWIDGET_H
#define MESSAGEEDITORWIDGET_H

#include <QtGui>

class Message;
class MessageTranslationOption;

class MessageEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageEditorWidget(QWidget *parent);

    void addTranslationOption(Message *message);

signals:

public slots:

private:
    std::vector<MessageTranslationOption *> m_trans;
    QHBoxLayout *m_layout;
};

#endif // MESSAGEEDITORWIDGET_H
