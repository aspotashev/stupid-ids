
#ifndef TRANSLATIONCONTENT_H
#define TRANSLATIONCONTENT_H

#include <gettext-po.h>

class TranslationContent
{
public:
	TranslationContent(const char *filename);
//	TranslationContent(GitLoader *git_loader, const git_oid *.../........);
	~TranslationContent();

	po_file_t poFileRead();

private:
	char *m_filename;
};

#endif // TRANSLATIONCONTENT_H

