#ifndef MESSAGEEDITORWIDGET_H
#define MESSAGEEDITORWIDGET_H

#include <QtGui>

class Message;
class MessageGroup;
class MessageTranslationOption;
class MessageHeaderWidget;

class MessageEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageEditorWidget(MessageGroup *messageGroup);

    void addTranslationOption(Message *message);
    void selectTranslationOption(MessageTranslationOption* option);
    bool isHighlighted(MessageTranslationOption* option);

signals:

public slots:

private:
    std::vector<MessageTranslationOption *> m_trans;
    QHBoxLayout *m_layout;
    MessageHeaderWidget *m_messageHeader;
    MessageTranslationOption* m_selection;
};

#endif // MESSAGEEDITORWIDGET_H
