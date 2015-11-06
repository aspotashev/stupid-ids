#include <gtpo/translationcontent.h>
#include <gtpo/filecontentfs.h>
#include <gtpo/gettextpo-helper.h>

#include <algorithm>
#include <iostream>

template <typename ForwardIt, typename ScoreFunc>
ForwardIt max_element_by_score(ForwardIt first, ForwardIt last, ScoreFunc func)
{
    // Return last if range is empty
    if (first == last)
        return last;

    ForwardIt maxIt = first;
    auto maxValue = func(*first);

    std::advance(first, 1);
    while (first != last) {
        auto value = func(*first);
        if (value > maxValue) {
            maxIt = first;
            maxValue = value;
        }

        std::advance(first, 1);
    }

    return maxIt;
}

int toolFindBestMatchingTranslated(int argc, char *argv[])
{
    const std::vector<std::string>& args = parseArgs(argc, argv);

    // At least 2 arguments:
    //   1st argument: the .po/.pot we want to find translations for
    //   2nd and further arguments: .po files with translations to choose from
    assert(args.size() >= 2);

    TranslationContent targetContent(new FileContentFs(args[0]));
    targetContent.readMessages();
//     targetContent.clearTranslations(); // TBD: may be we should keep the catalog partially translated so that we find the best matching source only for the untranslated strings?

    int baseTranslated = targetContent.translatedCount();

    // TBD: how to get max score using lambdas?
//     int maxScore = 0;
//     int maxScoreIndex = -1;
//     for (size_t i = 1; i < args.size(); ++i) {
//         TranslationContent content(args[i]);
//
//         // make a copy
//         TranslationContent tmp = targetContent;
//         tmp.copyTranslationsFrom(content);
//         ...
//     }

//     std::max_element();
    auto maxIt = max_element_by_score(
        args.begin() + 1, args.end(),
        [&targetContent,&baseTranslated](const std::string& file) {
            TranslationContent content(new FileContentFs(file));
            content.readMessages();

            // make a copy
            // TBD: copying should make a deep copy or use Copy-on-Write
            TranslationContent tmp = targetContent;
            tmp.copyTranslationsFrom(&content);

            tmp.writeToFile("/tmp/1.po", true);

            int score = tmp.translatedCount() - baseTranslated;

            if (score > 0)
                std::cout << score << "  \t" << file << std::endl;

            return score;
        });

    std::cout << "Best matching translated catalog: " << *maxIt << std::endl;
    return 0;
}
