
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <map>

#include "gitloader.h"
#include "detectorbase.h"
#include "processorphans.h"
#include "detector.h"


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

