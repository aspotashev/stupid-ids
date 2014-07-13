#include <gtpo/iddiff.h>

#include <iostream>

int main()
{
    Iddiffer* differ = new Iddiffer();
    differ->loadIddiff("1.iddiff");
    std::cout << differ->generateIddiffText();

    return 0;
}
