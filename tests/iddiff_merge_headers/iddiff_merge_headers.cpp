#include <gtpo/iddiff.h>

#include <assert.h>

int main()
{
    Iddiffer *diff_a = new Iddiffer();
    assert(diff_a->loadIddiff(INPUT_DATA_DIR "a.iddiff"));
    Iddiffer *diff_b = new Iddiffer();
    assert(diff_b->loadIddiff(INPUT_DATA_DIR "b.iddiff"));

    diff_a->mergeHeaders(diff_b);
    diff_a->writeToFile("result.iddiff");

    return 0; // ok
}
