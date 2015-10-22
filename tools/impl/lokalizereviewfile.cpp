#include "dbuslokalize.h"

#include <gtpo/translation-collector.h>
#include <gtpo/translationcontent.h>
#include <gtpo/gitloader.h>
#include <gtpo/iddiff.h>
#include <gtpo/gettextpo-helper.h>
#include <gtpo/message.h>
#include <gtpo/messagegroup.h>
#include <gtpo/iddiffmessage.h>

#include <unistd.h>

#include <cstdio>
#include <cstring>

// 1. Find the original .po from KDE SVN (GitLoader::findOldestByTphash, see also "tools/iddiff-repo")
// 2. Open the input .po file in Lokalize, select the original .po file for syncing (Lokalize D-Bus: openSyncSource)
// 3. Show only those strings that are different in two .po files (Lokalize D-Bus: setEntriesFilteredOut/setEntryFilteredOut)
int toolLokalizeReviewfile(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument
	const char *input_filename_relative = argv[1];

	// Calculate absolute path to file (input_filename might be a
	// relative path), so that Lokalize can find the file.
	char input_filename[1000];
	if (input_filename_relative[0] == '/') // absolute path
	{
		strcpy(input_filename, input_filename_relative);
	}
	else // relative path
	{
		assert(strlen(input_filename_relative) < 495);
		assert(getcwd(input_filename, 500) != NULL);
		strcat(input_filename, "/");
		strcat(input_filename, input_filename_relative);
	}


	TranslationContent *new_content = new TranslationContent(input_filename);
	GitOid tp_hash = new_content->calculateTpHash();
	assert(!tp_hash.isNull());

	// 1. Find the original .po from KDE SVN (GitLoader::findOldestByTphash, see also "tools/iddiff-repo")
	GitLoader *git_loader = new GitLoader();
	git_loader->addRepository("/home/sasha/kde-ru/kde-ru-trunk.git/.git");
	git_loader->addRepository("/home/sasha/kde-ru/kde-l10n-ru-stable/.git");
	TranslationContent *old_content = git_loader->findOldestByTphash(tp_hash.oid());

//	std::vector<int> upToDateIds = git_loader->getCurrentIdsVector();
//	sort(upToDateIds.begin(), upToDateIds.end());

	// Write old_content to temporary file
	const char *sync_filename = "/tmp/lokalize-sync-file.tmp";
	if (old_content)
	{
		old_content->setDisplayFilename("[sync]");
		old_content->writeBufferToFile(sync_filename);
	}

	// 2. Open the input .po file in Lokalize, select the original .po file for syncing (Lokalize D-Bus: openSyncSource)
	DBusLokalizeInterface lokalize;
	DBusLokalizeEditor *editor = lokalize.openFileInEditor(input_filename);
	if (old_content)
		editor->openSyncSource(sync_filename);

	// 3. Show only those strings that are different in two .po files (Lokalize D-Bus: setEntriesFilteredOut/setEntryFilteredOut)
	if (!old_content)
		return 0;

	std::vector<MessageGroup*> old_messages = old_content->readMessages();
	std::vector<MessageGroup*> new_messages = new_content->readMessages();
	size_t msg_count = old_messages.size();
	assert(msg_count == new_messages.size());

	editor->setEntriesFilteredOut(true);
	bool calledGotoEntry = false;

	for (size_t i = 0; i < msg_count; i ++)
	{
		Message* old_msg = old_messages[i]->message(0);
		Message* new_msg = new_messages[i]->message(0);

		if (!old_msg->equalTranslationsComments(new_msg) && (old_msg->isTranslated() || new_msg->isTranslated()))
		{
			if (!calledGotoEntry)
			{
				editor->gotoEntry(i);
				calledGotoEntry = true;
			}

			editor->setEntryFilteredOut(i, false);
			editor->clearTemporaryEntryNotes(i);

			IddiffMessage* iddiff_msg = new IddiffMessage();
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
