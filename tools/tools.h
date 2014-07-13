#ifndef TOOLS_H
#define TOOLS_H

int toolCalculateTpHash(int argc, char *argv[]);
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
int toolStupidsRerereTrustedFilter(int argc, char *argv[]);
int toolStupidsReverseTpHash(int argc, char *argv[]);

#endif // TOOLS_H
