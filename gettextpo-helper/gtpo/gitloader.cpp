#include "gitloader.h"
#include "gettextpo-helper.h"
#include "translationcontent.h"
#include "oidmapcache.h"
#include "stupids-client.h"
#include "repository.h"
#include "commit.h"
#include "commitfilechange.h"

#include <git2.h>

#include <vector>
#include <set>
#include <algorithm>
#include <stdexcept>

#include <cassert>

GitLoaderBase::~GitLoaderBase()
{
}

GitLoader::GitLoader()
{
}

GitLoader::~GitLoader()
{
    for (size_t i = 0; i < m_repos.size(); i ++)
        delete m_repos[i];
}

/**
 * \brief Search blob by OID in all repositories added using addRepository().
 *
 * \param oid Git object ID of the blob.
 *
 * \returns Blob or NULL, if it was not found in any of the repositories.
 * It is necessary to call the function "git_blob_free" when you
 * stop using a blob. Failure to do so will cause a memory leak.
 */
git_blob *GitLoader::blobLookup(const GitOid& oid)
{
    git_blob *blob;
    for (size_t i = 0; i < m_repos.size(); i ++)
        if ((blob = m_repos[i]->blobLookup(oid)))
            return blob;

    return NULL;
}

/**
 * \brief Add directory to the list of Git repositories to search in.
 *
 * The repository will be opened until GitLoader object is destroyed.
 */
void GitLoader::addRepository(const char *git_dir)
{
    m_repos.push_back(new Repository(git_dir));
}

const git_oid *GitLoader::findOldestByTphash_oid(const git_oid *tp_hash)
{
    // Cache results of this function (TODO: may be even create a stupids-server command for this function?)
    const git_oid *cached_oid = OidMapCacheManager::instance("lang_ru_oldest_oid").getValue(tp_hash);
    if (cached_oid)
        return cached_oid;

    // TODO: ch_iterator (iterator for walking through all CommitFileChanges)
    // TODO: better: "ch_iterator_time" -- walking through all CommitFileChanges sorted by time (BONUS: starting at the given time)
    for (size_t i = 0; i < m_repos.size(); i ++)
    {
        Repository *repo = m_repos[i];

        repo->readRepositoryCommits();
        // TODO: check only .po files committed after POT-Creation-Date?
        for (int j = 0; j < repo->nCommits(); j ++)
        {
            const Commit* commit = repo->commit(j);
            for (int k = 0; k < commit->nChanges(); k ++)
            {
                const CommitFileChange* change = commit->change(k);
                std::string name = change->name();
                if (name.substr(name.size() - 3) != ".po")
                    continue;

                const git_oid* oid = change->tryOid2();
                if (!oid)
                    continue;

                TranslationContent *content = new TranslationContent(this, oid);
                const git_oid *current_tp_hash = content->calculateTpHash();
                if (current_tp_hash && git_oid_cmp(tp_hash, current_tp_hash) == 0)
                {
                    delete content;
                    OidMapCacheManager::instance("lang_ru_oldest_oid").addPair(tp_hash, oid);
                    return oid; // TODO: choose the oldest TranslationContent from _all_ repositories
                }
                else
                {
                    delete content;
                }
            }
        }
    }

    return NULL;
}

TranslationContent *GitLoader::findOldestByTphash(const git_oid *tp_hash)
{
    const git_oid *oid = findOldestByTphash_oid(tp_hash);
    return oid ? new TranslationContent(this, oid) : NULL;
}

/**
 * \brief Returns an STL vector of IDs of all messages currently
 * present in "master" branch in any of the repositories.
 */
std::vector<int> GitLoader::getCurrentIdsVector()
{
    std::vector<GitOid> oids;
    for (size_t i = 0; i < m_repos.size(); i ++)
    {
        std::vector<GitOid> cur = m_repos[i]->getCurrentOids();
        for (size_t j = 0; j < cur.size(); j ++)
            oids.push_back(cur[j]);
    }

    std::vector<GitOid> tp_hashes;
    for (size_t i = 0; i < oids.size(); i ++)
    {
        TranslationContent *content = new TranslationContent(this, oids[i].oid());
        const git_oid *tp_hash = content->calculateTpHash();
        if (tp_hash)
            tp_hashes.push_back(GitOid(tp_hash));
        else
        {
            printf("Warning: unknown tp-hash\n");
            assert(0);
        }
    }

    std::vector<std::pair<int, int> > first_ids = stupidsClient.getFirstIdPairs(tp_hashes);

    std::vector<int> res;
    for (size_t i = 0; i < first_ids.size(); i ++)
    {
        int first_id = first_ids[i].first;
        int id_count = first_ids[i].second;

        if (first_id == 0)
            continue;
        assert(id_count >= 0);

        for (int id = first_id; id < first_id + id_count; id ++)
            res.push_back(id);
    }

    return res;
}
