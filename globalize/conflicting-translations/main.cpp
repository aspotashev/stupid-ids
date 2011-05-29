
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <gettextpo-helper/gettextpo-helper.h>

#include <QString>
#include <QDirIterator>

#include "../lib/qtranslationcollector.h"


// TODO: fix memory leaks
int main(int argc, char *argv[])
{
	TranslationCollector collector;

	collector.insertPoDir(QString("/home/sasha/kde-ru/kde-ru-trunk.git"));
	collector.insertPoDir(QString("/home/sasha/kde-ru/kde-l10n-ru-stable"));

//	collector.insertPo("/home/sasha/messages/kdebase/katesnippets_tng.po");
//	collector.insertPo("/home/sasha/stable-messages/kdesdk/katesnippets_tng.po");

	int num_shared = collector.numSharedIds();
	int num_total = collector.numIds();

	printf("Translation file match: %1.2lf%%\n", 100.0 * num_shared / num_total);

	std::vector<int> list = collector.listConflicting();
	for (int i = 0; i < (int)list.size(); i ++)
	{
		MessageGroup *variants = collector.listVariants(list[i]);
                printf("%d      msgid = [%s], msgid_plural = [%s], msgctxt = [%s]\n",
                    list[i], variants->msgid(), variants->msgidPlural(),
                    variants->msgctxt());

		for (int i = 0; i < variants->size(); i ++)
		{
			Message *msg = variants->message(i);
			printf("Variant %d: (from %s, #%d)\n", (int)i + 1, msg->filename(), msg->index() + 1);
			if (msg->isFuzzy())
				printf("\tfuzzy\n");
			printf("\tmsgcomments: %s\n", msg->msgcomments());
			printf("\tmsgstr: %s\n", msg->msgstr(0));
		}

		printf("----------------\n");
	}

	return 0;
}

