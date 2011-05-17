
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

int main()
{
	Repository *repo = new Repository("/home/sasha/kde-ru/xx-numbering/templates/.git/");
	Repository *repo_stable = new Repository("/home/sasha/kde-ru/xx-numbering/stable-templates/.git/");

	// Run detectors
	DetectorSuccessors d_succ(repo);
	d_succ.detect();
	printf("Detected pairs: %d\n", d_succ.nPairs());

	delete repo;
	delete repo_stable;

	return 0;
}

