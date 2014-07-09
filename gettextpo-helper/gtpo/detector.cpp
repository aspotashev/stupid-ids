
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

#include "detector.h"
#include "gitloader.h"
#include "processorphans.h"

DetectorSuccessors::DetectorSuccessors(Repository *repo, ProcessOrphansTxt *transitions = NULL):
    m_repo(repo), m_transitions(transitions)
{
}

DetectorSuccessors::~DetectorSuccessors()
{
}

void DetectorSuccessors::processChange(int commit_index, int change_index, const CommitFileChange *change)
{
    const git_oid *oid;
    std::vector<const ProcessOrphansTxtEntry *> transitions;

    switch (change->type())
    {
    case CommitFileChange::MOD:
        addOidPair(change->oid1(), change->oid2());
        break;
    case CommitFileChange::DEL:
        // Load commands "move" and "merge" from ProcessOrphansTxt and search forward
        if (m_transitions)
        {
            m_transitions->findByOrigin(transitions, change->name(), change->path(), ProcessOrphansTxtEntry::MOVE | ProcessOrphansTxtEntry::MERGE);
            for (size_t i = 0; i < transitions.size(); i ++)
            {
                const ProcessOrphansTxtEntry *entry = transitions[i];

                oid = m_repo->findNextUpdateOid(std::max(commit_index - 20, 0), entry->destNamePot(), entry->destPath());
                if (oid)
                {
                    addOidPair(change->oid1(), oid);
//                  printf("yay! [%s]\n", change->name());
                }
            }
        }
        break;
    case CommitFileChange::ADD:
        oid = m_repo->findLastRemovalOid(commit_index - 1, change->name(), change->path());
        if (oid)
            addOidPair(oid, change->oid2());

        // Load commands "copy" from ProcessOrphansTxt and search backwards
        if (m_transitions)
        {
            m_transitions->findByDestination(transitions, change->name(), change->path(), ProcessOrphansTxtEntry::COPY);
            for (size_t i = 0; i < transitions.size(); i ++)
            {
                const ProcessOrphansTxtEntry *entry = transitions[i];

                // Try to find where the added file was copied from
                oid = m_repo->findLastUpdateOid(commit_index, entry->origNamePot(), entry->origPath());
                if (oid)
                {
                    addOidPair(change->oid2(), oid);
//                  printf("yay! [%s/%s] -> [%s/%s], nCommits - commit_index = %d\n",
//                      entry->origPath(), entry->origNamePot(),
//                      entry->destPath(), entry->destNamePot(),
//                      m_repo->nCommits() - commit_index);
                }
            }
        }
        break;
    // TBD: handle "mergekeep" command somehow
    default:
        assert(0);
    }
}

void DetectorSuccessors::doDetect()
{
    assert(m_repo);
    m_repo->readRepositoryCommits();

    for (int i = 0; i < m_repo->nCommits(); i ++)
    {
        const Commit *commit = m_repo->commit(i);
        for (int j = 0; j < commit->nChanges(); j ++)
            processChange(i, j, commit->change(j));
    }
}

//-------------------------------------------------------

DetectorInterBranch::DetectorInterBranch(Repository *repo_a, Repository *repo_b):
    m_repoA(repo_a), m_repoB(repo_b)
{
}

DetectorInterBranch::~DetectorInterBranch()
{
}

// commit_index and change_index are related to repo (not other_repo)
void DetectorInterBranch::processChange(
    Repository *repo, Repository *other_repo,
    int commit_index, int change_index, const CommitFileChange *change)
{
    const git_oid *oid = change->tryOid2();
    if (oid == NULL)
        return;

    const git_oid *other_oid = other_repo->findFileOidByTime(repo->commit(commit_index)->time(), change->name(), change->path());
    if (other_oid)
        addOidPair(oid, other_oid);

    // "+5000" was put here for reliability ;)
    // This should cope with system time instability on the KDE SVN server.
    //
    //
    // TODO: also search for renamed files (from process_orphans.txt)
    // For example, "STABLE/messages/kdesdk/katesnippets_tng.po" is the same as "TRUNK/messages/kdebase/katesnippets_tng.po"
    const git_oid *other_oid2 = other_repo->findFileOidByTime(repo->commit(commit_index)->time() + 5000, change->name(), change->path());
    if (other_oid2 && other_oid2 != other_oid)
        addOidPair(oid, other_oid2);
}

void DetectorInterBranch::doDetect()
{
    assert(m_repoA);
    assert(m_repoB);
    m_repoA->readRepositoryCommits();
    m_repoB->readRepositoryCommits();

    for (int i = 0; i < m_repoA->nCommits(); i ++)
    {
        const Commit *commit = m_repoA->commit(i);
        for (int j = 0; j < commit->nChanges(); j ++)
            processChange(m_repoA, m_repoB, i, j, commit->change(j));
    }
}

//-------------------------------------------------------

GraphCondenser::GraphCondenser(int n)
{
    m_n = n;
    m_edges = new std::vector<int>[n];
    m_vis = new bool[n];
    for (int i = 0; i < n; i ++)
    {
        m_edges[i] = std::vector<int>();
        m_vis[i] = false;
    }
}

GraphCondenser::~GraphCondenser()
{
    delete [] m_edges;
    delete [] m_vis;
}

// Adds bidirectional edge
void GraphCondenser::addEdge(int i, int j)
{
    m_edges[i].push_back(j);
    m_edges[j].push_back(i);
}

void GraphCondenser::dumpCluster(std::vector<int> &dest, int start = -1)
{
    if (start == -1)
    {
        for (int i = 0; i < m_n; i ++)
            if (!m_vis[i])
            {
                start = i;
                break;
            }

        // If all vertices are visited, exit
        if (start == -1)
            return;
    }

    if (!m_vis[start])
    {
        dest.push_back(start);
        m_vis[start] = true;

        for (size_t i = 0; i < m_edges[start].size(); i ++)
            dumpCluster(dest, m_edges[start][i]);
    }
}

//-------------------------------------------------------

void clusterStats(Repository *repo, Repository *repo_stable, std::vector<GitOidPair> &allPairs)
{
    // Generating statistics on "clusters" of OIDs
    std::vector<GitOid2Change> allOids;
    repo->dumpOids(allOids);
    repo_stable->dumpOids(allOids);

    sort(allOids.begin(), allOids.end());
    allOids.resize(unique(allOids.begin(), allOids.end()) - allOids.begin());

    printf("Number of unique OIDs: %d\n", (int)allOids.size());

    // Factorization
    std::map<GitOid2Change, int> gitoid2num;
    for (size_t i = 0; i < allOids.size(); i ++)
        gitoid2num[allOids[i]] = i;

    // Condense a graph with the given number of vertices
    GraphCondenser cond(allOids.size());
    for (size_t i = 0; i < allPairs.size(); i ++)
    {
        cond.addEdge(
            gitoid2num[GitOid2Change(allPairs[i].oid1(), NULL)],
            gitoid2num[GitOid2Change(allPairs[i].oid2(), NULL)]);
    }

    while (1)
    {
        std::vector<int> cluster;
        cond.dumpCluster(cluster);
        if (cluster.size() == 0)
            break;

//      if (!strcmp(allOids[cluster[0]].change()->name(), "words.pot") || !strcmp(allOids[cluster[0]].change()->name(), "kword.pot"))
//      if (!strcmp(allOids[cluster[0]].change()->name(), "katesnippets_tng.pot"))
        if (cluster.size() < 3)
        {
            printf("%d\n", (int)cluster.size());
            for (size_t i = 0; i < cluster.size(); i ++)
            {
                const CommitFileChange *change = allOids[cluster[i]].change();
                printf("%s/%s\n", change->path(), change->name());
            }
        }
    }

}

void generateGraphviz(std::vector<GitOidPair> &allPairs)
{
    FILE *f_dot = fopen("1.dot", "w");
    fprintf(f_dot, "graph identified_translation_templates\n{\n");
    char oid1_str[41] = {0};
    char oid2_str[41] = {0};
    for (size_t i = 0; i < allPairs.size(); i ++)
    {
        git_oid_fmt(oid1_str, allPairs[i].oid1());
        git_oid_fmt(oid2_str, allPairs[i].oid2());

        fprintf(f_dot, "    \"%s\" -- \"%s\";\n", oid1_str, oid2_str);
    }
    fprintf(f_dot, "}\n");
    fclose(f_dot);
}

//-------------------------------------------------------

void writePairToFile(std::vector<GitOidPair> &allPairs, const char *out_file)
{
    FILE *fout = fopen(out_file, "wb");
    for (size_t i = 0; i < allPairs.size(); i ++)
    {
        assert(fwrite(allPairs[i].oid1(), GIT_OID_RAWSZ, 1, fout) == 1);
        assert(fwrite(allPairs[i].oid2(), GIT_OID_RAWSZ, 1, fout) == 1);
    }

    fclose(fout);
}

//-------------------------------------------------------

void detectTransitions(std::vector<GitOidPair> &dest, const char *path_trunk, const char *path_stable, const char *path_proorph)
{
    Repository *repo = new Repository(path_trunk);
    Repository *repo_stable = new Repository(path_stable);
    ProcessOrphansTxt transitions(path_proorph);

    // Run detectors
    std::vector<DetectorBase *> detectors;
    detectors.push_back(new DetectorSuccessors(repo, &transitions));
    detectors.push_back(new DetectorSuccessors(repo_stable, &transitions));
    detectors.push_back(new DetectorInterBranch(repo, repo_stable));

    std::vector<GitOidPair> allPairs;
    for (size_t i = 0; i < detectors.size(); i ++)
    {
        detectors[i]->detect();
        printf("Detected pairs: %d\n", detectors[i]->nPairs());

        detectors[i]->dumpPairs(allPairs);

        delete detectors[i];
        detectors[i] = NULL;
    }

    printf("\nTotal pairs: %d\n", (int)allPairs.size());
    sort(allPairs.begin(), allPairs.end());
    allPairs.resize(unique(allPairs.begin(), allPairs.end()) - allPairs.begin());
    printf("Number of unique pairs: %d\n", (int)allPairs.size());

    // TODO: do this using STL, or even use 'unique_copy' instead of 'unique'
    for (size_t i = 0; i < allPairs.size(); i ++)
        dest.push_back(allPairs[i]);

//  writePairToFile(allPairs, "pairs.dat");
//  clusterStats(repo, repo_stable, allPairs);
//  generateGraphviz(allPairs);


    delete repo;
    delete repo_stable;
}

//-------------------------------------------------------

void filterProcessedTransitions(const char *file_processed, std::vector<GitOidPair> &input, std::vector<GitOidPair> &output)
{
    FILE *f = fopen(file_processed, "rb");
    if (!f)
        throw std::runtime_error("Failed to open file " + std::string(file_processed));

    fseek(f, 0, SEEK_END);
    int n_processed = ftell(f) / (2 * GIT_OID_RAWSZ);
    rewind(f);


    unsigned char oid1_raw[GIT_OID_RAWSZ];
    unsigned char oid2_raw[GIT_OID_RAWSZ];
    git_oid oid1;
    git_oid oid2;
    std::set<GitOidPair> processed;
    for (int i = 0; i < n_processed; i ++)
    {
        assert(fread(oid1_raw, GIT_OID_RAWSZ, 1, f) == 1);
        assert(fread(oid2_raw, GIT_OID_RAWSZ, 1, f) == 1);

        git_oid_fromraw(&oid1, oid1_raw);
        git_oid_fromraw(&oid2, oid2_raw);

        processed.insert(GitOidPair(&oid1, &oid2));
    }

    fclose(f);

    for (size_t i = 0; i < input.size(); i ++)
        if (processed.find(input[i]) == processed.end())
            output.push_back(input[i]);
}

