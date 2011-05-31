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
    void setHighlight(bool highlighted);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    MessageEditorWidget *m_editor;
    Message *m_message;
    bool m_highlighted;
};

#endif // MESSAGETRANSLATIONOPTION_H
