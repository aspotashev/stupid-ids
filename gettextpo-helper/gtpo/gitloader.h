#ifndef GITLOADER_H
#define GITLOADER_H

#include <gtpo/detectorbase.h>
#include <gtpo/filedatetime.h>

#include <git2.h>

#include <vector>
#include <stack>

class Repository;

class GitLoaderBase
{
public:
    virtual ~GitLoaderBase();

    virtual git_blob* blobLookup(const GitOid& oid) = 0;
};

//----------------------------------------

class TranslationContent;

/** Class for loading files by their Git OIDs from multiple Git repositories.
 */
class GitLoader : public GitLoaderBase
{
public:
    GitLoader();
    ~GitLoader();

    virtual git_blob *blobLookup(const GitOid& oid);
    void addRepository(const char *git_dir);

    // Finds the oldest (in Git repositories' histories) .po file with the given tp_hash.
    TranslationContent *findOldestByTphash(const GitOid& tp_hash);

    std::vector<int> getCurrentIdsVector();

private:
    GitOid findOldestByTphash_oid(const GitOid& tp_hash);

private:
    std::vector<Repository*> m_repos;
};

// char *concat_path(const std::string& path, const std::string& name);
bool ends_with(const std::string& str, const std::string& ending);
bool is_dot_or_dotdot(const std::string& str);

#endif // GITLOADER_H
