#include <gtpo/iddiff.h>

#include <assert.h>

// TODO: merge into "tools/iddiff-merge" (as optional mode)
int main(int argc, char *argv[])
{
	assert(argc == 3); // 2 arguments
	const char *src_path = argv[1];
	const char *merged_path = argv[2];

	Iddiffer *src_diff = new Iddiffer();
	if (!src_diff->loadIddiff(src_path)) // src_path does not exist
		return 1;

	Iddiffer *merged_diff = new Iddiffer();
	if (merged_diff->loadIddiff(merged_path)) // there was a file
	{
		merged_diff->mergeTrComments(src_diff);
		merged_diff->writeToFile(merged_path);
	}
	else // merged_diff did not exist
	{
		src_diff->writeToFile(merged_path);
	}

	return 0;
}

