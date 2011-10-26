
#include <stdio.h>
#include <set>
#include <sstream>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/detectorbase.h>
#include <dbuslokalize.h>
#include <sys/stat.h>

class GreedySetCover
{
public:
    /**
    * From all \a TranslationContent objects from \a collector selects a
    * minimal set of them that contains messages with all IDs mentioned in
    * \a diff.
    */
	GreedySetCover(StupIdTranslationCollector *collector, Iddiffer *diff);

    /**
     * Performs the next step of the greedy algorithm.
     */
	TranslationContent *nextContent();

private:
    // TranslationContent-s that can still be returned by "nextContent()".
    // TODO: use std::list instead of std::vector
	std::vector<TranslationContent *> m_invContents;

    // Minimized IDs that have not been covered yet by
    // any TranslationContent already returned by "nextContent()".
	std::set<int> m_ids;
};

GreedySetCover::GreedySetCover(StupIdTranslationCollector *collector, Iddiffer *diff)
{
    // Minimizing "ids". See comment in nextContent() after "if (m_ids.find(c_ids[j]) != m_ids.end())".
	diff->minimizeIds();

	// Initializing "m_ids".
	std::vector<int> ids = diff->involvedIds();
	m_ids = std::set<int>(ids.begin(), ids.end());

	// Initializing "m_invContents".
	m_invContents = collector->involvedByMinIds(ids);
}

TranslationContent *GreedySetCover::nextContent()
{
	int best_index = -1;
	int best_covered = 0;

	for (size_t i = 0; i < m_invContents.size(); i ++)
	{
		TranslationContent *content = m_invContents[i];
		const std::vector<int> &c_ids = content->getMinIds();

		int covered = 0;
		for (int j = 0; j < c_ids.size(); j ++)
			if (m_ids.find(c_ids[j]) != m_ids.end()) // We are searching for a _minimized_ ID "c_ids[j]",
				covered ++;                          // therefore all IDs in "m_ids" should also be minimized.

		if (covered > best_covered)
		{
			best_index = i;
			best_covered = covered;
		}
	}

	if (best_index == -1)
		return NULL;

	// Remove handled IDs from m_ids
	TranslationContent *content = m_invContents[best_index];
	std::vector<int> c_ids = content->getMinIds();
	for (int i = 0; i < c_ids.size(); i ++)
		m_ids.erase(c_ids[i]);

	// Remove handled content from the list
	m_invContents.erase(m_invContents.begin() + best_index);

	return content;
}


int main(int argc, char *argv[])
{
	assert(argc == 2); // 1 argument
	const char *input_iddiff = argv[1];

	DBusLokalizeInterface lokalize;

	StupIdTranslationCollector collector;
	collector.insertPoDirOrTemplate(
		"/home/sasha/kde-ru/xx-numbering/templates",
		"/home/sasha/kde-ru/kde-ru-trunk.git");
	collector.insertPoDirOrTemplate(
		"/home/sasha/kde-ru/xx-numbering/stable-templates",
		"/home/sasha/kde-ru/kde-l10n-ru-stable");

	Iddiffer *diff = new Iddiffer();
	diff->loadIddiff(input_iddiff);
	GreedySetCover cover(&collector, diff);

	TranslationContent *content = NULL;
	while (content = cover.nextContent())
	{
		printf("Reviewing content: %s\n", content->displayFilename());

		std::ostringstream tempdir;
		tempdir << "/tmp/lk-iddiff-" << (int)time(NULL);
		tempdir << "_" << GitOid(content->gitBlobHash()).toString();
		std::string tempdir_str = tempdir.str();

        assert(mkdir(tempdir_str.c_str(), 0777) == 0);

		const char *file_a = xstrdup((tempdir_str + std::string("/") + path_to_basename(content->displayFilename())).c_str());
		const char *file_b = xstrdup((tempdir_str + "/merge_from.po").c_str());

		content->writeToFile(file_a, true);
		diff->applyToContent(content);
		content->writeToFile(file_b, true);

		DBusLokalizeEditor *editor = lokalize.openFileInEditor(file_a);
		editor->openSyncSource(file_b);

		editor->setEntriesFilteredOut(true);

		std::vector<MessageGroup *> messages = content->readMessages();
		bool gotoEntryCalled = false;
		for (size_t i = 0; i < messages.size(); i ++)
			if (messages[i]->message(0)->isEdited())
			{
				editor->setEntryFilteredOut(i, false);
				if (!gotoEntryCalled)
				{
					editor->gotoEntry(i);
					gotoEntryCalled = true;
				}
			}
	}

	return 0;
}

