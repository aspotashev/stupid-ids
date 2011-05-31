#include "messageeditorwidget.h"
#include "messagetranslationoption.h"

MessageEditorWidget::MessageEditorWidget(QWidget *parent):
    QWidget(parent)
{
    setMinimumHeight(30);

    m_layout = new QHBoxLayout(this);
    m_layout->setMargin(0);
}

void MessageEditorWidget::addTranslationOption(Message *message)
{
    MessageTranslationOption *trans = new MessageTranslationOption(this, message);
    m_trans.push_back(trans);
    m_layout->addWidget(trans);
}
