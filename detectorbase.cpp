
#include "detectorbase.h"

GitOidPair::GitOidPair(const git_oid *oid1, const git_oid *oid2)
{
	setPair(oid1, oid2);
}

GitOidPair::~GitOidPair()
{
}

void GitOidPair::setPair(const git_oid *oid1, const git_oid *oid2)
{
	git_oid_cpy(&m_oid1, oid1);
	git_oid_cpy(&m_oid2, oid2);
}

//GitOidPair &GitOidPair::operator=(const GitOidPair &o)
//{
//      ...
//}

//----------------------------------------

DetectorBase::DetectorBase()
{
	m_nPairs = 0;

	m_maxPairs = 20000;
	m_oidPairs = (GitOidPair *)malloc(sizeof(GitOidPair) * m_maxPairs);
}

DetectorBase::~DetectorBase()
{
	free(m_oidPairs);
}

void DetectorBase::addOidPair(const git_oid *oid1, const git_oid *oid2)
{
	if (m_nPairs == m_maxPairs)
	{
		m_maxPairs += 20000;
		m_oidPairs = (GitOidPair *)realloc(m_oidPairs, sizeof(GitOidPair) * m_maxPairs);
	}

	int cmp = git_oid_cmp(oid1, oid2);
	if (cmp < 0)
		m_oidPairs[m_nPairs ++].setPair(oid1, oid2);
	else if (cmp > 0)
		m_oidPairs[m_nPairs ++].setPair(oid2, oid1);
	else
	{
		// Strange, we are identifying a file with itself
//		assert(0);
	}
}

void DetectorBase::detect()
{
	// TODO: benchmarking
	doDetect();
}

int DetectorBase::nPairs() const
{
	return m_nPairs;
}

