// TODO: i18n of labels in the output (e.g. "Suggested translation:")
// TODO: write strings with "Corrected translation:" after all strings without "Corrected translation:"

#include <assert.h>
#include <stdio.h>
#include <iostream>

#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/iddiff.h>
#include <gettextpo-helper/gitloader.h>


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
	input_translation->setDisplayFilename(input_translation_path); // TODO: do this automatically in TranslationContent ctor
	std::vector<int> min_ids = input_translation->getMinIds();
	std::vector<MessageGroup *> messages = input_translation->readMessages(false); // TODO: loadObsolete=false by default
	assert(min_ids.size() == messages.size()); // TODO: move this assertion into class TranslationContent

	int review_item_index = 1;
	for (size_t i = 0; i < min_ids.size(); i ++)
	{
		int min_id = min_ids[i];
		MessageGroup *messageGroup = messages[i];
		Message *message = messageGroup->message(0);

		IddiffMessage *added = iddiff->findAddedSingle(min_id);
		std::vector<IddiffMessage *> removed = iddiff->findRemoved(min_id);

		// TODO: also skip messages that have not been changed by
		// this translator in this "translation session" (irrelevant
		// for newly created .po files). See tools/lokalize-reviewfile for
		// the necessary filtering technique.

		// Skip message if the submitted translation (from "input_translation")
		// was not marked as "REMOVED" in the iddiff, i.e. the message looks OK to
		// the reviewer or has not been reviewed yet.
		bool ignoreOldTranslation = message->isUntranslated(); // TODO: copied from Iddiffer::applyToMessage, go there and follow TODOs
		for (size_t j = 0; !ignoreOldTranslation && j < removed.size(); j ++) // TODO: make this a function; TODO: implement this through findRemoved (when Message and IddiffMessage will be merged)
			if (removed[j]->equalTranslations(message))
				ignoreOldTranslation = true;
		if (!ignoreOldTranslation)
			continue;


		std::cout << review_item_index << ". Строка перевода №" << (i + 1) << std::endl;

		std::cout << "   Исходная строка: " << IddiffMessage::formatString(messageGroup->msgid()) << std::endl;
		if (messageGroup->msgidPlural())
			std::cout << "   Множественное число: " << IddiffMessage::formatString(messageGroup->msgidPlural()) << std::endl;

		std::cout << "   Предложенный перевод: " << message->formatPoMessage() << std::endl;
		if (added)
			std::cout << "   Исправленный перевод: " << added->formatPoMessage() << std::endl;

		if (iddiff->reviewCommentText(min_id))
			std::cout << std::endl << "   " << iddiff->reviewCommentText(min_id) << std::endl << std::endl;

		review_item_index ++;
	}

	return 0;
}

