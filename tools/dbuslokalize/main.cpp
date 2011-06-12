
#include <stdio.h>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gettextpo-helper.h>

#include "dbuslokalize.h"

int main(int argc, char *argv[])
{
	DBusLokalizeInterface lokalize;

	StupIdTranslationCollector collector;
//	collector.insertPoDir("/home/sasha/kde-ru/kde-ru-trunk.git");
//	collector.insertPoDir("/home/sasha/kde-ru/kde-l10n-ru-stable");
	collector.insertPoDir("/home/sasha/kde-ru/kde-ru-trunk.git/messages/extragear-multimedia");

	Iddiffer *diff = new Iddiffer();
	diff->loadIddiff("/home/sasha/stupid-ids/tools/iddiff/amarok.iddiff");
	diff->minimizeIds();
	std::vector<int> ids = diff->involvedIds();

	std::map<int, std::vector<MessageGroup *> > inv_messages; // involved messages
	std::vector<TranslationContent *> inv_contents; // involved contents (not used)
	collector.getMessagesByIds(inv_messages, inv_contents, ids);

	std::map<std::string, DBusLokalizeEditor *> filename2editor;
	for (size_t i = 0; i < ids.size(); i ++)
	{
		std::vector<MessageGroup *> messages = inv_messages[ids[i]];
		if (messages.size() == 0)
		{
			printf("ID not found: %d\n", ids[i]);
			continue;
		}

		// Open only one message for every ID
		MessageGroup *messageGroup = messages[0];
		assert(messageGroup->size() > 0);
		Message *message = messageGroup->message(0);

		std::string filename = std::string(message->filename());
		DBusLokalizeEditor *editor;
		if (filename2editor.find(filename) == filename2editor.end())
		{
			editor = filename2editor[filename] = lokalize.openFileInEditor(filename.c_str());
			editor->setEntriesFilteredOut(true);
			editor->gotoEntry(message->index());
		}

		editor = filename2editor[filename];
		editor->setEntryFilteredOut(message->index(), false);
		editor->addTemporaryEntryNote(message->index(), "Hello, world!");
	}

	return 0;
}

