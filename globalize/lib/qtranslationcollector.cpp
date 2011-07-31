
#include "qtranslationcollector.h"

#include <assert.h>

#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtCore/QDirIterator>

void TranslationCollector::insertPo(QString path)
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

	StupIdTranslationCollector::insertPo(buffer, (size_t)len, fn);
}

void TranslationCollector::insertPoDir(QString directory_path)
{
	QDirIterator directory_walker(directory_path, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
	while (directory_walker.hasNext())
	{
		directory_walker.next();
		if (directory_walker.fileInfo().completeSuffix() == QString("po"))
			insertPo(directory_walker.filePath());
	}
}

