
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <translation-collector.h>
#include <gettextpo-helper.h>


int main(int argc, char *argv[])
{
	StupIdTranslationCollector collector;
	collector.insertPo("a1.po");
	collector.insertPo("a2.po");

	std::vector<int> list = collector.listConflicting();
	for (int i = 0; i < (int)list.size(); i ++)
	{
		printf("%d\n", list[i]);
		std::vector<Message *> variants = collector.listVariants(list[i]);

		for (size_t i = 0; i < variants.size(); i ++)
		{
			Message *msg = variants[i];
			printf("Variant %d:\n", (int)i + 1);
			if (msg->isFuzzy())
				printf("\tfuzzy\n");
			printf("\tmsgctxt: %s\n", msg->msgcomments());
			printf("\tmsgstr: %s\n", msg->msgstr(0));
		}

		printf("----------------\n");
	}

	return 0;
}

