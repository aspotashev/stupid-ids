#ifndef MESSAGEHEADERWIDGET_H
#define MESSAGEHEADERWIDGET_H

#include <QtGui/QWidget>


class MessageEditorWidget;
class Message;
class MessageGroup;
class QPaintEvent;

class MessageHeaderWidget : public QWidget
{
public:
    explicit MessageHeaderWidget(MessageEditorWidget *editor, MessageGroup *messageGroup);
    virtual ~MessageHeaderWidget();
    
protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    MessageEditorWidget *m_editor;
    MessageGroup *m_messageGroup;
};

#endif // MESSAGEHEADERWIDGET_H
