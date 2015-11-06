#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>
#include <gtpo/message.h>
#include <gtpo/messagegroup.h>

int main()
{
    TranslationContent* content = new TranslationContent(new FileContentFs("data/input.po"));
    std::vector<MessageGroup*> messages = content->readMessages();

    Message *trans = messages[0]->message(0);
    trans->editMsgstr(0, std::string("xx") + std::string(trans->msgstr(0)) + std::string("xx"));

    content->writeToFile();

    return 0;
}
