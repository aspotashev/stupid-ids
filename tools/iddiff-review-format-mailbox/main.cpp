// TODO: i18n of labels in the output (e.g. "Suggested translation:")

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>

class ReviewMailEntry
{
public:
	ReviewMailEntry(int index, Iddiffer *iddiff, MessageGroup *messageGroup, int min_id);

	std::string generateText(int review_item_index);
	bool hasCorrected() const;
	bool operator<(const ReviewMailEntry &o) const;

private:
	int m_index;
	Iddiffer *m_iddiff;
	MessageGroup *m_messageGroup;
	int m_minId;
};

ReviewMailEntry::ReviewMailEntry(int index, Iddiffer *iddiff, MessageGroup *messageGroup, int min_id):
	m_index(index),
	m_iddiff(iddiff),
	m_messageGroup(messageGroup),
	m_minId(min_id)
{
}

bool ReviewMailEntry::hasCorrected() const
{
	return m_iddiff->findAddedSingle(m_minId) != NULL;
}

bool ReviewMailEntry::operator<(const ReviewMailEntry &o) const
{
	return hasCorrected() == false && o.hasCorrected() == true;
}

std::string ReviewMailEntry::generateText(int review_item_index)
{
	std::stringstream out(std::stringstream::out);
	Message *message = m_messageGroup->message(0);
	IddiffMessage *added = m_iddiff->findAddedSingle(m_minId);

	out << review_item_index << ". Строка перевода №" << (m_index + 1) << std::endl;

	out << "   Исходная строка: " << IddiffMessage::formatString(m_messageGroup->msgid()) << std::endl;
	if (m_messageGroup->msgidPlural())
		out << "   Множественное число: " << IddiffMessage::formatString(m_messageGroup->msgidPlural()) << std::endl;

	out << "   Предложенный перевод: " << message->formatPoMessage() << std::endl;
	if (added)
		out << "   Исправленный перевод: " << added->formatPoMessage() << std::endl;

	if (m_iddiff->reviewCommentText(m_minId))
		out << std::endl << "   " << m_iddiff->reviewCommentText(m_minId) << std::endl;

	return out.str();
}


int main(int argc, char *argv[])
{
//	if (argc != 3) // 2 arguments
//	{
//		fprintf(stderr, "Example usage:\n\tiddiff-git ./.git 7fb8df3aed979214165e7c7d28e672966b13a15b\n");
//		exit(1);
//	}

	const char *iddiff_path = "/home/sasha/.stupids/lokalize-review-test.iddiff";
	const char *input_translation_path = "/home/sasha/docmessages/kdebase-workspace/kdm_theme-ref.po";

	Iddiffer *iddiff = new Iddiffer();
	iddiff->loadIddiff(iddiff_path);
	iddiff->minimizeIds(); // patching should always be done with minimized IDs

	TranslationContent *input_translation = new TranslationContent(input_translation_path);
	std::vector<int> min_ids = input_translation->getMinIds();
	std::vector<MessageGroup *> messages = input_translation->readMessages();

	std::vector<ReviewMailEntry> mail_entries;
	for (size_t i = 0; i < min_ids.size(); i ++)
	{
		int min_id = min_ids[i];
		MessageGroup *messageGroup = messages[i];
		Message *message = messageGroup->message(0);

		// TODO: also skip messages that have not been changed by
		// this translator in this "translation session" (irrelevant
		// for newly created .po files). See tools/lokalize-reviewfile for
		// the necessary filtering technique.

		// Skip message if the submitted translation (from "input_translation")
		// was not marked as "REMOVED" in the iddiff, i.e. the message looks OK to
		// the reviewer or has not been reviewed yet.
		if (!iddiff->canDropMessage(message, min_id))
			continue;

		mail_entries.push_back(ReviewMailEntry(i, iddiff, messageGroup, min_id));
	}

	// Messages without "Corrected translation:" should go first
	sort(mail_entries.begin(), mail_entries.end());

	for (size_t i = 0; i < mail_entries.size(); i ++)
	{
		if (mail_entries[i].hasCorrected() && (i == 0 || !mail_entries[i - 1].hasCorrected()))
			std::cout << "===== Строки с готовыми исправлениями переводов =====" << std::endl;

		std::cout << mail_entries[i].generateText(i) << std::endl;
	}

	return 0;
}

