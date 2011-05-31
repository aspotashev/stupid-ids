#include "messagetranslationoption.h"

#include <assert.h>
#include <gettextpo-helper/detectorbase.h>
#include <gettextpo-helper/gettextpo-helper.h>

MessageTranslationOption::MessageTranslationOption(MessageEditorWidget *editor, Message *message):
    QWidget(), m_editor(editor), m_message(message)
{
    assert(message);

    setMinimumWidth(50);

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    setLayout(layout);


    QPalette pal_fuzzy;
    pal_fuzzy.setColor(QPalette::WindowText, Qt::gray);

    QLabel *label;

    label = new QLabel(QString::fromUtf8(message->msgstr(0)));
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    if (message->isFuzzy())
        label->setPalette(pal_fuzzy);
    layout->addWidget(label, 0, 0);

    label = new QLabel(QString::fromUtf8(message->msgcomments()));
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    if (message->isFuzzy())
        label->setPalette(pal_fuzzy);
    layout->addWidget(label, 1, 0);
}

MessageTranslationOption::~MessageTranslationOption()
{
}

void MessageTranslationOption::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    // TODO: clicking on this widget should select this translation among others
}

void MessageTranslationOption::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);
    QRectF border = rect();
    border.setBottom(border.bottom() - 1);
    border.setRight(border.right() - 1);
    p.drawRoundedRect(border, 5, 5);
}
