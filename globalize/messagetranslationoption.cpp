#include "messagetranslationoption.h"

#include <gettextpo-helper/detectorbase.h>

MessageTranslationOption::MessageTranslationOption(MessageEditorWidget *editor, Message *message):
    QWidget(), m_editor(editor), m_message(message)
{
    setMinimumWidth(50);

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    setLayout(layout);
    
    layout->addWidget(new QPushButton("123"), 0, 0);
}

MessageTranslationOption::~MessageTranslationOption()
{
}

void MessageTranslationOption::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    // TODO: clicking on this widget should select this translation among others
}
