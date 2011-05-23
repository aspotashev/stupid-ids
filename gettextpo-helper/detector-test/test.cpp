
#include <gettextpo-helper/detector.h>

int main()
{
	std::vector<GitOidPair> allPairs;
	detectTransitions(
		allPairs,
		"/home/sasha/kde-ru/xx-numbering/templates/.git/",
		"/home/sasha/kde-ru/xx-numbering/stable-templates/.git/",
		"/home/sasha/l10n-kde4/scripts/process_orphans.txt");

	return 0;
}

