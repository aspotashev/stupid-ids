#ifndef MESSAGETRANSLATIONOPTION_H
#define MESSAGETRANSLATIONOPTION_H

#include <QtGui>

class Message;
class MessageEditorWidget;

class MessageTranslationOption : public QWidget
{
    Q_OBJECT

public:
    MessageTranslationOption(MessageEditorWidget *editor, Message *message);
    ~MessageTranslationOption();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    
private:
    MessageEditorWidget *m_editor;
    Message *m_message;
};

#endif // MESSAGETRANSLATIONOPTION_H
