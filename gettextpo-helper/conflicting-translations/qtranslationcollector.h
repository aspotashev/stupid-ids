
#include <gettextpo-helper/translation-collector.h>

#include <QString>

class TranslationCollector : public StupIdTranslationCollector
{
public:
	void insertPo(QString path);
	void insertPoDir(QString directory_path);
};

