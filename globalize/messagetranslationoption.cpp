#include "messagetranslationoption.h"
#include "messageeditorwidget.h"

#include <assert.h>
#include <gtpo/detectorbase.h>
#include <gtpo/message.h>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPainter>

MessageTranslationOption::MessageTranslationOption(MessageEditorWidget* editor, Message* message)
    : QWidget()
    , m_editor(editor)
    , m_message(message)
{
    assert(message);

    m_highlighted = false;

    setMinimumWidth(50);

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    setLayout(layout);


    QPalette pal_fuzzy;
    pal_fuzzy.setColor(QPalette::WindowText, Qt::gray);

    QLabel *label;

    label = new QLabel(QString::fromStdString(message->msgstr(0)));
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    if (message->isFuzzy())
        label->setPalette(pal_fuzzy);
    layout->addWidget(label, 0, 0);

    label = new QLabel(QString::fromStdString(message->msgcomments()));
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

    // If this option is selected, then remove selection.
    // If this option is not selected, then select it.
    m_editor->selectTranslationOption(m_editor->isHighlighted(this) ? NULL : this);
}

void MessageTranslationOption::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);
    if (m_highlighted)
        p.setBrush(QBrush(QColor(210, 230, 255)));
    QRectF border = rect();
    border.setBottom(border.bottom() - 1);
    border.setRight(border.right() - 1);
    p.drawRoundedRect(border, 5, 5);
}

void MessageTranslationOption::setHighlight(bool highlighted)
{
    if (highlighted == m_highlighted)
        return;

    m_highlighted = highlighted;
    update();
}
