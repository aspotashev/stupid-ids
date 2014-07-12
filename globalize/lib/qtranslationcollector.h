#include <gtpo/translation-collector.h>

#include <QtCore/QString>

class TranslationCollector : public StupIdTranslationCollector
{
public:
	void insertPo(QString path);
	void insertPoDir(QString directory_path);
};
