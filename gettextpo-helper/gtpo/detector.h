
#ifndef DETECTOR_H
#define DETECTOR_H

#include <gtpo/detectorbase.h>

class Repository;
class ProcessOrphansTxt;

class DetectorSuccessors : public DetectorBase
{
public:
    DetectorSuccessors(Repository *repo, ProcessOrphansTxt *transitions);
    ~DetectorSuccessors();

protected:
    void processChange(int commit_index, int change_index, const CommitFileChange *change);
    virtual void doDetect();

private:
    Repository *m_repo;
    ProcessOrphansTxt *m_transitions;
};

//-------------------------------------------------------

class DetectorInterBranch : public DetectorBase
{
public:
    DetectorInterBranch(Repository *repo_a, Repository *repo_b);
    ~DetectorInterBranch();

protected:
    void processChange(
        Repository *repo, Repository *other_repo,
        int commit_index, int change_index, const CommitFileChange *change);
    virtual void doDetect();

private:
    Repository *m_repoA;
    Repository *m_repoB;
};

//-------------------------------------------------------

class GraphCondenser
{
public:
    GraphCondenser(int n);
    ~GraphCondenser();

    void addEdge(int i, int j);
    void dumpCluster(std::vector<int> &dest, int start);

private:
    int m_n;
    std::vector<int> *m_edges;
    bool *m_vis;
};

//-------------------------------------------------------

void clusterStats(Repository *repo, Repository *repo_stable, std::vector<GitOidPair> &allPairs);
void generateGraphviz(std::vector<GitOidPair> &allPairs);

void writePairToFile(std::vector<GitOidPair> &allPairs, const char *out_file);

void detectTransitions(std::vector<GitOidPair> &dest, const char *path_trunk, const char *path_stable, const char *path_proorph);

void filterProcessedTransitions(const char *file_processed, std::vector<GitOidPair> &input, std::vector<GitOidPair> &output);

#endif // DETECTOR_H

