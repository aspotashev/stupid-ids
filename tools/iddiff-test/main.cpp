
#include <iostream>

#include <gettextpo-helper/iddiff.h>


int main()
{
	Iddiffer *differ = new Iddiffer();
	differ->loadIddiff("1.iddiff");
	std::cout << differ->generateIddiffText();

	return 0;
}

