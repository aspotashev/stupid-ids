#include "messageheaderwidget.h"

#include <gettextpo-helper/message.h>

MessageHeaderWidget::MessageHeaderWidget(MessageEditorWidget *editor, MessageGroup *messageGroup):
    QWidget(), m_editor(editor), m_messageGroup(messageGroup)
{
    setMinimumWidth(100);
    setMaximumWidth(100);


    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    setLayout(layout);

    QLabel *label;

    label = new QLabel(QString::fromUtf8(messageGroup->msgid()));
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    layout->addWidget(label, 0, 0);

    label = new QLabel(QString::fromUtf8(messageGroup->msgctxt()));
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    layout->addWidget(label, 1, 0);
}

MessageHeaderWidget::~MessageHeaderWidget()
{
}

void MessageHeaderWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    QPainter p(this);
    p.setPen(Qt::blue);
    QRectF border = rect();
    border.setBottom(border.bottom() - 1);
    border.setRight(border.right() - 1);
    p.drawRoundedRect(border, 5, 5);
}
