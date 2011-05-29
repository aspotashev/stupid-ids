
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



void insertPo(StupIdTranslationCollector *collector, QString path)
{
	QFile file(path);
	assert(file.open(QIODevice::ReadOnly));

	int len = (int)file.size();
	assert(file.size() == (qint64)len); // file should be small enough

	char *buffer = new char[len];
	assert(buffer);

	QDataStream ds(&file);
	assert(ds.readRawData(buffer, len) == len);
	file.close();


	// only for naming
	QByteArray ba = path.toUtf8();
	const char *fn = ba.data();
	printf("%s\n", fn);

	collector->insertPo(buffer, (size_t)len, fn);
}

void insertPoDir(StupIdTranslationCollector *collector, QString directory_path)
{
	QDirIterator directory_walker(directory_path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (directory_walker.hasNext())
	{
		directory_walker.next();
		if (directory_walker.fileInfo().completeSuffix() == QString("po"))
			insertPo(collector, directory_walker.filePath());
	}
}

int main(int argc, char *argv[])
{
	StupIdTranslationCollector collector;
//	collector.insertPo("a1.po");
//	collector.insertPo("a2.po");

//	collector.insertPo("/home/sasha/messages/kdebase/dolphin.po");
//	collector.insertPo("/home/sasha/stable-messages/kdebase/dolphin.po");

//	collector.insertPo("/home/sasha/messages/calligra/words.po");
//	collector.insertPo("/home/sasha/messages/koffice/kword.po");

	insertPoDir(&collector, "/home/sasha/messages/kdepim");
	insertPoDir(&collector, "/home/sasha/stable-messages/kdepim");

//	collector.insertPo("/home/sasha/messages/kdebase/katesnippets_tng.po");
//	collector.insertPo("/home/sasha/stable-messages/kdesdk/katesnippets_tng.po");

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

