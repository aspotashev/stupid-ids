#include "messageeditorwidget.h"
#include "messagetranslationoption.h"
#include "messageheaderwidget.h"
#include <gettextpo-helper/gettextpo-helper.h>

#include <QtGui/QHBoxLayout>

MessageEditorWidget::MessageEditorWidget(MessageGroup *messageGroup):
    QWidget()
{
    m_selection = NULL;

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

void MessageEditorWidget::selectTranslationOption(MessageTranslationOption* option)
{
    if (m_selection == option)
        return;

    if (m_selection)
        m_selection->setHighlight(false);

    m_selection = option;
    if (option)
    {
        option->setHighlight(true);
    }
}

bool MessageEditorWidget::isHighlighted(MessageTranslationOption* option)
{
    return m_selection == option;
}
