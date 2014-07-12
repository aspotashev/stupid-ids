#include "../lib/qtranslationcollector.h"

#include <gtpo/gettextpo-helper.h>
#include <gtpo/message.h>
#include <gtpo/messagegroup.h>

#include <QString>
#include <QDirIterator>

#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <assert.h>

// TODO: fix memory leaks
int main(int argc, char *argv[])
{
	TranslationCollector collector;

	collector.insertPoDir(QString("/home/sasha/kde-ru/kde-ru-trunk.git"));
	collector.insertPoDir(QString("/home/sasha/kde-ru/kde-l10n-ru-stable"));

//	collector.insertPo("/home/sasha/messages/kdebase/katesnippets_tng.po");
//	collector.insertPo("/home/sasha/stable-messages/kdesdk/katesnippets_tng.po");

	std::vector<int> list = collector.listConflicting();
	for (int i = 0; i < (int)list.size(); i ++)
	{
		MessageGroup* variants = collector.listVariants(list[i]);
                printf("%d      msgid = [%s], msgid_plural = [%s], msgctxt = [%s]\n",
                    list[i], variants->msgid().c_str(), variants->msgidPlural().c_str(),
                    variants->msgctxt().c_str());

		for (int i = 0; i < variants->size(); i ++)
		{
			Message *msg = variants->message(i);
			printf("Variant %d: (from %s, #%d)\n",
                               (int)i + 1, msg->filename().c_str(), msg->index() + 1);
			if (msg->isFuzzy())
				printf("\tfuzzy\n");
			printf("\tmsgcomments: %s\n", msg->msgcomments().c_str());
			printf("\tmsgstr: %s\n", msg->msgstr(0).c_str());
		}

		printf("----------------\n");
	}

	return 0;
}
