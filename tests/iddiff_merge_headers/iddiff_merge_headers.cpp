#include <gtpo/iddiff.h>

#include <assert.h>

int main()
{
    Iddiff *diff_a = new Iddiff();
    assert(diff_a->loadIddiff(INPUT_DATA_DIR "a.iddiff"));
    Iddiff *diff_b = new Iddiff();
    assert(diff_b->loadIddiff(INPUT_DATA_DIR "b.iddiff"));

    diff_a->mergeHeaders(diff_b);
    diff_a->writeToFile("result.iddiff");

    return 0; // ok
}
