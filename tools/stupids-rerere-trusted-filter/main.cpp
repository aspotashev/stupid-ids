
#include <iostream>

#include <gettextpo-helper/iddiff.h>


int main(int argc, char *argv[])
{
	assert(argc == 3); // 2 arguments
	const char *rerere_path = argv[1];
	const char *input_path = argv[2];

	Iddiffer *input_diff = new Iddiffer();
	if (!input_diff->loadIddiff(input_path)) // input_path does not exist
		return 1;

	Iddiffer *rerere_diff = new Iddiffer();
	if (!rerere_diff->loadIddiff(rerere_path)) // rerere not found, cannot filter
		return 2;

	Iddiffer *filtered = new Iddiffer();
	filtered->filterTrustedIddiff(rerere_diff, input_diff);
	std::cout << filtered->generateIddiffText();

	return 0;
}

