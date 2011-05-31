#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "messageeditorwidget.h"

#include <gettextpo-helper/gettextpo-helper.h>
#include "lib/qtranslationcollector.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(m_layout);


    // Conflicting translations
    TranslationCollector collector;

    collector.insertPoDir(QString("/home/sasha/kde-ru/kde-ru-trunk.git/messages/kdepim"));
    collector.insertPoDir(QString("/home/sasha/kde-ru/kde-l10n-ru-stable/messages/kdepim"));

    std::vector<int> list = collector.listConflicting();
    for (int i = 0; i < (int)list.size(); i ++)
    {
            MessageGroup *variants = collector.listVariants(list[i]);
            printf("%d      msgid = [%s], msgid_plural = [%s], msgctxt = [%s]\n",
                list[i], variants->msgid(), variants->msgidPlural(),
                variants->msgctxt());

            MessageEditorWidget *editor = addMessageEditor();

            for (int i = 0; i < variants->size(); i ++)
            {
                    Message *msg = variants->message(i);
                    printf("Variant %d: (from %s, #%d)\n", (int)i + 1, msg->filename(), msg->index() + 1);
                    if (msg->isFuzzy())
                            printf("\tfuzzy\n");
                    printf("\tmsgcomments: %s\n", msg->msgcomments());
                    printf("\tmsgstr: %s\n", msg->msgstr(0));

                    editor->addTranslationOption(msg);
            }

            printf("----------------\n");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

MessageEditorWidget* MainWindow::addMessageEditor()
{
    MessageEditorWidget *editor = new MessageEditorWidget(ui->scrollAreaWidgetContents);
    m_layout->addWidget(editor);

    return editor;
}
