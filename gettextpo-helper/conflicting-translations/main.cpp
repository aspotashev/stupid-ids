
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <gettextpo-helper/translation-collector.h>
#include <gettextpo-helper/gettextpo-helper.h>

#include <QString>
#include <QDirIterator>


int main(int argc, char *argv[])
{
	StupIdTranslationCollector collector;
//	collector.insertPo("a1.po");
//	collector.insertPo("a2.po");

	collector.insertPo("/home/sasha/messages/kdebase/dolphin.po");
	collector.insertPo("/home/sasha/stable-messages/kdebase/dolphin.po");

//	collector.insertPo("/home/sasha/messages/calligra/words.po");
//	collector.insertPo("/home/sasha/messages/koffice/kword.po");

	QString directory_path("/home/sasha/messages/kdelibs");
	QDirIterator directory_walker(directory_path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (directory_walker.hasNext())
	{
		directory_walker.next();
		if (directory_walker.fileInfo().completeSuffix() == QString("po"))
		{
			QByteArray ba = directory_walker.filePath().toUtf8();
			const char *fn = ba.data();
			printf("%s\n", fn);

			collector.insertPo(fn);
		}
	}

	int num_shared = collector.numSharedIds();
	int num_total = collector.numIds();

	printf("Translation file match: %1.2lf%%\n", 100.0 * num_shared / num_total);

	std::vector<int> list = collector.listConflicting();
	for (int i = 0; i < (int)list.size(); i ++)
	{
		printf("%d\n", list[i]);
		std::vector<Message *> variants = collector.listVariants(list[i]);

		for (size_t i = 0; i < variants.size(); i ++)
		{
			Message *msg = variants[i];
			printf("Variant %d: (from %s, #%d)\n", (int)i + 1, msg->filename(), msg->index() + 1);
			if (msg->isFuzzy())
				printf("\tfuzzy\n");
			printf("\tmsgctxt: %s\n", msg->msgcomments());
			printf("\tmsgstr: %s\n", msg->msgstr(0));
		}

		printf("----------------\n");
	}

	return 0;
}

