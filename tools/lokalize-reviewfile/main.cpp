
#include <stdio.h>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gettextpo-helper.h>

#include "dbuslokalize.h"

// 1. Find the original .po from KDE SVN (GitLoader::findOldestByTphash, see also "tools/iddiff-repo")
// 2. Open the input .po file in Lokalize, select the original .po file for syncing (Lokalize D-Bus: openSyncSource)
// 3. Show only those strings that are different in two .po files (Lokalize D-Bus: setEntriesFilteredOut/setEntryFilteredOut)
int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument
	const char *input_filename = argv[1];

	TranslationContent *new_content = new TranslationContent(input_filename);
	new_content->setDisplayFilename(input_filename);
	const git_oid *tp_hash = new_content->calculateTpHash();
	assert(tp_hash);

	// 1. Find the original .po from KDE SVN (GitLoader::findOldestByTphash, see also "tools/iddiff-repo")
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");
	TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash);
	assert(old_content); // TODO: old_content may be NULL if nobody tried translating the given .po file before.
	old_content->setDisplayFilename("[sync]");

	// Write old_content to temporary file
	const char *sync_filename = "/tmp/lokalize-sync-file.tmp";
	old_content->writeBufferToFile(sync_filename);

	// 2. Open the input .po file in Lokalize, select the original .po file for syncing (Lokalize D-Bus: openSyncSource)
	DBusLokalizeInterface lokalize;
	DBusLokalizeEditor *editor = lokalize.openFileInEditor(input_filename);
	editor->openSyncSource(sync_filename);

	// 3. Show only those strings that are different in two .po files (Lokalize D-Bus: setEntriesFilteredOut/setEntryFilteredOut)
	std::vector<MessageGroup *> old_messages = old_content->readMessages(false);
	std::vector<MessageGroup *> new_messages = new_content->readMessages(false);
	size_t msg_count = old_messages.size();
	assert(msg_count == new_messages.size());

	editor->setEntriesFilteredOut(true);
	bool calledGotoEntry = false;

	for (size_t i = 0; i < msg_count; i ++)
	{
		Message *old_msg = old_messages[i]->message(0);
		Message *new_msg = new_messages[i]->message(0);

		if (!old_msg->equalTranslations(new_msg) && (old_msg->isTranslated() || new_msg->isTranslated()))
		{
			if (!calledGotoEntry)
			{
				editor->gotoEntry(i);
				calledGotoEntry = true;
			}

			editor->setEntryFilteredOut(i, false);
			editor->clearTemporaryEntryNotes(i);

			IddiffMessage *iddiff_msg = new IddiffMessage();
			for (int j = 0; j < old_msg->numPlurals(); j ++)
				iddiff_msg->addMsgstr(old_msg->msgstr(j));

			std::string note;
			note += std::string("Previous translation");
			if (old_msg->isFuzzy())
				note += std::string(" [FUZZY]");
			note += std::string(": ");
			note += iddiff_msg->formatPoMessage();

			editor->addTemporaryEntryNote(i, note.c_str());
		}
	}

	return 0;
}

