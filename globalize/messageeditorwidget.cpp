#include "messageeditorwidget.h"
#include "messagetranslationoption.h"
#include "messageheaderwidget.h"
#include <gettextpo-helper/gettextpo-helper.h>

MessageEditorWidget::MessageEditorWidget(MessageGroup *messageGroup):
    QWidget()
{
    setMinimumHeight(30);

    m_layout = new QHBoxLayout(this);
    m_layout->setMargin(0);

    m_messageHeader = new MessageHeaderWidget(this, messageGroup);
    m_layout->addWidget(m_messageHeader);
}

void MessageEditorWidget::addTranslationOption(Message *message)
{
    MessageTranslationOption *trans = new MessageTranslationOption(this, message);
    m_trans.push_back(trans);
    m_layout->addWidget(trans);
}
