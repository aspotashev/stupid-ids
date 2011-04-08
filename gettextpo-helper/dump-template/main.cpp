
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <gettext-po.h>

#include "../include/gettextpo-helper.h"


int main(int argc, char *argv[])
{
	const char *filename = NULL;
	if (argc == 1)
		filename = "-";
	else if (argc == 2)
		filename = argv[1];
	else
		assert(0); // too many arguments


	std::cout << calculate_tp_hash(filename);

	return 0;
}

