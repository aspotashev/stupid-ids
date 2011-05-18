
#include <stdio.h>
#include <assert.h>
#include <algorithm>

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
	const git_oid *oid = NULL;

	// TODO: create a method in class CommitFileChange for this
	switch (change->type())
	{
	case CommitFileChange::MOD:
	case CommitFileChange::ADD:
		oid = change->oid2();
		break;
	case CommitFileChange::DEL:
		break;
	default:
		assert(0);
	}

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

	delete repo;
	delete repo_stable;

	return 0;
}

