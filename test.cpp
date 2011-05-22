
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <map>

#include "gitloader.h"
#include "detectorbase.h"
#include "processorphans.h"

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

DetectorSuccessors::DetectorSuccessors(Repository *repo, ProcessOrphansTxt *transitions = NULL):
	m_repo(repo), m_transitions(transitions)
{
}

DetectorSuccessors::~DetectorSuccessors()
{
}

void DetectorSuccessors::processChange(int commit_index, int change_index, const CommitFileChange *change)
{
	const git_oid *oid1;

	switch (change->type())
	{
	case CommitFileChange::MOD:
		// TODO: Try to apply data from ProcessOrphansTxt here
		addOidPair(change->oid1(), change->oid2());
		break;
	case CommitFileChange::DEL:
		// Do nothing
		break;
	case CommitFileChange::ADD:
		// TODO: Try to apply data from ProcessOrphansTxt here
		oid1 = m_repo->findLastRemovalOid(commit_index - 1, change->name(), change->path());
		if (oid1)
			addOidPair(oid1, change->oid2());
		break;
	default:
		assert(0);
	}
}

void DetectorSuccessors::doDetect()
{
	for (int i = 0; i < m_repo->nCommits(); i ++)
	{
		const Commit *commit = m_repo->commit(i);
		for (int j = 0; j < commit->nChanges(); j ++)
			processChange(i, j, commit->change(j));
	}
}

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

	// For reliability ;)
	// This should cope with system time instability on the KDE SVN server.
	const git_oid *other_oid2 = other_repo->findFileOidByTime(repo->commit(commit_index)->time() + 5000, change->name(), change->path());
	if (other_oid2 && other_oid2 != other_oid)
		addOidPair(oid, other_oid2);
}

void DetectorInterBranch::doDetect()
{
	for (int i = 0; i < m_repoA->nCommits(); i ++)
	{
		const Commit *commit = m_repoA->commit(i);
		for (int j = 0; j < commit->nChanges(); j ++)
			processChange(m_repoA, m_repoB, i, j, commit->change(j));
	}
}

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

int main()
{
	Repository *repo = new Repository("/home/sasha/kde-ru/xx-numbering/templates/.git/");
	Repository *repo_stable = new Repository("/home/sasha/kde-ru/xx-numbering/stable-templates/.git/");
	ProcessOrphansTxt transitions("/home/sasha/l10n-kde4/scripts/process_orphans.txt");

	// Run detectors
	std::vector<DetectorBase *> detectors;
	detectors.push_back(new DetectorSuccessors(repo, &transitions));
	detectors.push_back(new DetectorSuccessors(repo_stable, &transitions));
	detectors.push_back(new DetectorInterBranch(repo, repo_stable));

	std::vector<GitOidPair> allPairs;
	for (size_t i = 0; i < detectors.size(); i ++)
	{
		DetectorBase *detector = detectors[i];
		detector->detect();
		printf("Detected pairs: %d\n", detector->nPairs());

		detector->dumpPairs(allPairs);
	}

	printf("\nTotal pairs: %d\n", (int)allPairs.size());
	sort(allPairs.begin(), allPairs.end());
	allPairs.resize(unique(allPairs.begin(), allPairs.end()) - allPairs.begin());
	printf("Number of unique pairs: %d\n", (int)allPairs.size());

//	clusterStats(repo, repo_stable, allPairs);
//	generateGraphviz(allPairs);


	delete repo;
	delete repo_stable;

	return 0;
}

