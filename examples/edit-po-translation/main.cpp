#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/message.h>

int main()
{
	TranslationContent *content = new TranslationContent("data/input.po");
	std::vector<MessageGroup *> messages = content->readMessages();

	Message *trans = messages[0]->message(0);
	trans->editMsgstr(0, ("xx" + std::string(trans->msgstr(0)) + "xx").c_str());

	content->writeToFile();

	return 0;
}
