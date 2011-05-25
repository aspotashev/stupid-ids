
#include "gettextpo-helper.h"
#include "translationcontent.h"

TranslationContent::TranslationContent(const char *filename)
{
	m_filename = xstrdup(filename);
}

TranslationContent::~TranslationContent()
{
	if (m_filename)
		delete [] m_filename;
}

po_file_t TranslationContent::poFileRead()
{
	assert(m_filename);

	return po_file_read(m_filename);
}

