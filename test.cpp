
#include <stdio.h>
#include <assert.h>

#include "gitloader.h"
#include "detectorbase.h"

class DetectorSuccessors : public DetectorBase
{
public:
	DetectorSuccessors(Repository *repo);
	~DetectorSuccessors();

protected:
	void processChange(int commit_index, int change_index, const CommitFileChange *change);
	virtual void doDetect();

private:
	Repository *m_repo;
};

DetectorSuccessors::DetectorSuccessors(Repository *repo):
	m_repo(repo)
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
		addOidPair(change->oid1(), change->oid2());
		break;
	case CommitFileChange::DEL:
		// Do nothing
		break;
	case CommitFileChange::ADD:
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

	// Run detectors
	std::vector<DetectorBase *> detectors;
	detectors.push_back(new DetectorSuccessors(repo));
	detectors.push_back(new DetectorInterBranch(repo, repo_stable));

	for (size_t i = 0; i < detectors.size(); i ++)
	{
		DetectorBase *detector = detectors[i];
		detector->detect();
		printf("Detected pairs: %d\n", detector->nPairs());
	}

	delete repo;
	delete repo_stable;

	return 0;
}

