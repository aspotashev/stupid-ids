#include <gtpo/iddiff.h>

#include <iostream>

int toolRerereTrustedFilter(int argc, char *argv[])
{
    assert(argc >= 3); // at least 2 arguments
    const char *input_path = argv[1];

    Iddiff *input_diff = new Iddiff();
    if (!input_diff->loadIddiff(input_path)) {
        // input_path does not exist
        return 1;
    }

    for (int i = 2; i < argc; ++i) {
        const char *rerere_path = argv[i];

        Iddiff *rerere_diff = new Iddiff();
        if (!rerere_diff->loadIddiff(rerere_path)) {
            // rerere not found, cannot filter
            return 2;
        }

        Iddiff *filtered = new Iddiff();
        filtered->filterTrustedIddiff(rerere_diff, input_diff);
        delete rerere_diff;

        delete input_diff;
        input_diff = filtered;
    }

    std::cout << input_diff->generateIddiffText();

    return 0;
}
