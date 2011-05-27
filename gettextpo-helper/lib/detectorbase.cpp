
#include <assert.h>

#include <gettextpo-helper/detectorbase.h>

GitOid::GitOid()
{
	const static unsigned char zero_oid_raw[GIT_OID_RAWSZ] = {0};

	git_oid_mkraw(&m_oid, zero_oid_raw);
}

GitOid::GitOid(const git_oid *oid)
{
	setOid(oid);
}

GitOid::~GitOid()
{
}

void GitOid::setOid(const git_oid *oid)
{
	git_oid_cpy(&m_oid, oid);
}

bool GitOid::operator<(const GitOid &o) const
{
	return git_oid_cmp(&m_oid, &o.m_oid) < 0;
}

bool GitOid::operator==(const GitOid &o) const
{
	return git_oid_cmp(&m_oid, &o.m_oid) == 0;
}

//----------------------------------------

GitOid2Change::GitOid2Change():
	m_change(NULL)
{
}

GitOid2Change::GitOid2Change(const git_oid *oid, const CommitFileChange *change):
	GitOid(oid), m_change(change)
{
}

GitOid2Change::~GitOid2Change()
{
}

const CommitFileChange *GitOid2Change::change() const
{
	return m_change;
}

//----------------------------------------

GitOidPair::GitOidPair()
{
	const static unsigned char zero_oid_raw[GIT_OID_RAWSZ] = {0};

	git_oid_mkraw(&m_oid1, zero_oid_raw);
	git_oid_mkraw(&m_oid2, zero_oid_raw);
}

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

bool GitOidPair::operator<(const GitOidPair &o) const
{
	assert(git_oid_cmp(&m_oid1, &m_oid2) < 0);

	int cmp1 = git_oid_cmp(&m_oid1, &o.m_oid1);
	if (cmp1 < 0)
		return true;
	else if (cmp1 > 0)
		return false;
	else
		return git_oid_cmp(&m_oid2, &o.m_oid2) < 0;
}

bool GitOidPair::operator==(const GitOidPair &o) const
{
	assert(git_oid_cmp(&m_oid1, &m_oid2) < 0);

	return git_oid_cmp(&m_oid1, &o.m_oid1) == 0 && git_oid_cmp(&m_oid2, &o.m_oid2) == 0;
}

const git_oid *GitOidPair::oid1() const
{
	return &m_oid1;
}

const git_oid *GitOidPair::oid2() const
{
	return &m_oid2;
}

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

void DetectorBase::dumpPairs(std::vector<GitOidPair> &dest) const
{
	for (int i = 0; i < m_nPairs; i ++)
		dest.push_back(m_oidPairs[i]);
}

