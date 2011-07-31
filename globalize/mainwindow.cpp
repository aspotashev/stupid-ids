#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "messageeditorwidget.h"

#include <stdio.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/message.h>
#include "lib/qtranslationcollector.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(m_layout);

    //loadConflicting();


//---------------------------------------------------------------------------
    // Preparing message list for review
    TranslationCollector collector;
    collector.insertPoDir(QString("/home/sasha/kde-ru/kde-ru-trunk.git"));
//    collector.insertPoDir(QString("/home/sasha/kde-ru/kde-l10n-ru-stable"));

    // Reading iddiff for review
    Iddiffer *diff = new Iddiffer();
    diff->loadIddiff("/home/sasha/stupid-ids/tools/iddiff/amarok.iddiff");
    diff->minimizeIds();
    std::vector<int> ids = diff->involvedIds();

    std::map<int, std::vector<MessageGroup *> > inv_messages; // involved messages
    std::vector<TranslationContent *> inv_contents; // involved contents (not used)
    collector.getMessagesByIds(inv_messages, inv_contents, ids);
    for (size_t i = 0; i < ids.size(); i ++)
    {
        int id = ids[i];

        std::vector<MessageGroup *> messages = inv_messages[id];

        if (messages.size() == 0)
        {
            printf("ID not found: %d\n", id);
            continue;
        }
        MessageEditorWidget *editor = addMessageEditor(messages[0]);

        for (size_t i = 0; i < messages.size(); i ++)
        {
            assert(messages[i]->size() == 1);
            Message *msg = messages[i]->message(i);

            editor->addTranslationOption(msg);
        }

        std::vector<IddiffMessage *> removed = diff->findRemoved(id);
        for (size_t i = 0; i < removed.size(); i ++)
        {
            Message *msg = new Message(false, messages[0]->size(), "");
            for (size_t j = 0; j < messages[0]->size(); j ++)
                msg->setMsgstr(j, "");
            removed[i]->copyTranslationsToMessage(msg);

            editor->addTranslationOption(msg);
        }

        IddiffMessage *added = diff->findAddedSingle(id);
        if (added)
        {
            Message *msg = new Message(false, messages[0]->size(), "");
            for (size_t j = 0; j < messages[0]->size(); j ++)
                msg->setMsgstr(j, "");
            added->copyTranslationsToMessage(msg);

            editor->addTranslationOption(msg);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

MessageEditorWidget* MainWindow::addMessageEditor(MessageGroup *messageGroup)
{
    MessageEditorWidget *editor = new MessageEditorWidget(messageGroup);
    m_layout->addWidget(editor);

    return editor;
}

void MainWindow::loadConflicting()
{
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

        MessageEditorWidget *editor = addMessageEditor(variants);

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
