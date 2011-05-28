
// http://www.gnu.org/software/gettext/manual/gettext.html#libgettextpo
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <gettextpo-helper.h>

// escape quotes (" -> \") and put in quotes
std::string iddiff_format_string(const char *str)
{
	assert(str);

	std::string res;

	res += "\""; // opening quote

	size_t len = strlen(str);
	for (size_t i = 0; i < len; i ++)
	{
		if (str[i] == '\"')
			res += "\\\""; // escape quote
		else
			res += str[i];
	}

	res += "\"";
	return res;
}

std::string iddiff_format_po_message(po_message_t message)
{
	std::string res;

	if (po_message_is_fuzzy(message))
		res += "f";

	int n_plurals = po_message_n_plurals(message);
	if (n_plurals > 0) // message with plurals
	{
		res += iddiff_format_string(po_message_msgstr_plural(message, 0));
		for (int i = 1; i < n_plurals; i ++)
		{
			res += " "; // separator
			res += iddiff_format_string(po_message_msgstr_plural(message, i));
		}
	}
	else // message without plurals
	{
		res += iddiff_format_string(po_message_msgstr(message));
	}

	return res;
}

void print_message_list(std::vector<std::pair<int, std::string> > list)
{
	for (size_t i = 0; i < list.size(); i ++)
		printf("%d %s\n", list[i].first, list[i].second.c_str());
}

int main(int argc, char *argv[])
{
	assert(argc == 4); // 3 arguments

	const char *filename_a = argv[1];
	const char *filename_b = argv[2];
	int first_id = atoi(argv[3]); // it is the same for 2 files

	// .po files should be derived from the same .pot
	assert(calculate_tp_hash(filename_a) == calculate_tp_hash(filename_b));


	// compare pairs of messages in 2 .po files
	po_file_t file_a = po_file_read(filename_a);
	po_file_t file_b = po_file_read(filename_b);

	po_message_iterator_t iterator_a =
		po_message_iterator(file_a, "messages");
	po_message_iterator_t iterator_b =
		po_message_iterator(file_b, "messages");
	// skipping headers
	po_message_t message_a = po_next_message(iterator_a);
	po_message_t message_b = po_next_message(iterator_b);

	std::vector<std::pair<int, std::string> > removed_list;
	std::vector<std::pair<int, std::string> > added_list;

	for (int index = 0;
		(message_a = po_next_message(iterator_a)) &&
		(message_b = po_next_message(iterator_b)) &&
		!po_message_is_obsolete(message_a) &&
		!po_message_is_obsolete(message_b);
		index ++)
	{
		if (strcmp(po_message_comments(message_a), po_message_comments(message_b)))
		{
			printf("Changes in comments will be ignored!");
			printf("<<<<<\n");
			printf("%s", po_message_comments(message_a)); // "\n" should be included in comments
			printf("=====\n");
			printf("%s", po_message_comments(message_b)); // "\n" should be included in comments
			printf(">>>>>\n");
		}


		// Messages can be:
		//     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
		//     "abc" -- translated
		//     f"abc" -- fuzzy

		// Types of possible changes:
		//     ""     -> ""     : -
		//     ""     -> "abc"  : ADDED
		//     ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
		//     "abc"  -> ""     : REMOVED
		//     "abc"  -> "abc"  : -
		//     "abc"  -> f"abc" : REMOVED
		//     f"abc" -> ""     : REMOVED (removing fuzzy messages is OK)
		//     f"abc" -> "abc"  : REMOVED, ADDED
		//     f"abc" -> f"abc" : -
		//     "abc"  -> "def"  : REMOVED, ADDED
		//     "abc"  -> f"def" : REMOVED
		//     f"abc" -> "def"  : REMOVED, ADDED
		//     f"abc" -> f"def" : REMOVED

		// If there were no changes (i.e. message_a == message_b)
		if (!compare_po_message_msgstr(message_a, message_b))
			continue;

		// Adding to "REMOVED" if:
		//    "A" is not untranslated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_a))
			removed_list.push_back(std::pair<int, std::string>(
				first_id + index, iddiff_format_po_message(message_a)));

		// Adding to "ADDED" if:
		//    "B" is translated & there were changes (i.e. message_a != message_b)
		if (!po_message_is_untranslated(message_b) && !po_message_is_fuzzy(message_b))
			added_list.push_back(std::pair<int, std::string>(
				first_id + index, iddiff_format_po_message(message_b)));
	}

	// free memory
	po_message_iterator_free(iterator_a);
	po_message_iterator_free(iterator_b);
	po_file_free(file_a);
	po_file_free(file_b);

	if (removed_list.size() > 0 || added_list.size() > 0)
	{
		printf("Subject: \n");
		printf("Author: \n");
		printf("Date: \n");
		printf("\n");
		printf("REMOVED\n");
		print_message_list(removed_list);
		printf("ADDED\n");
		print_message_list(added_list);
	}

	return 0;
}

