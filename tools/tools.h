#ifndef TOOLS_H
#define TOOLS_H

int toolIddiff(int argc, char *argv[]);
int toolIddiffGit(int argc, char *argv[]);
int toolIddiffMerge(int argc, char *argv[]);
int toolIddiffMergeTrcomm(int argc, char *argv[]);
int toolIddiffMinimizeIds(int argc, char *argv[]);
int toolIddiffRepo(int argc, char *argv[]);
int toolIddiffReviewFormatMailbox(int argc, char *argv[]);
int toolIdpatch(int argc, char *argv[]);
int toolLokalizeReviewIddiff(int argc, char *argv[]);
int toolLokalizeReviewfile(int argc, char *argv[]);
int toolRerereTrustedFilter(int argc, char *argv[]);
int toolReverseTpHash(int argc, char *argv[]);
int toolFindBestMatchingTranslated(int argc, char *argv[]);

#endif // TOOLS_H
