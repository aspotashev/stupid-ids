
#ifndef DETECTORBASE_H
#define DETECTORBASE_H

#include <vector>

#include <git2.h>

class GitOid
{
public:
	GitOid();
	GitOid(const git_oid *oid);
	~GitOid();

	void setOid(const git_oid *oid);

	bool operator<(const GitOid &o) const;
	bool operator==(const GitOid &o) const;

private:
	git_oid m_oid;
};

class GitOidPair
{
public:
	GitOidPair();
	GitOidPair(const git_oid *oid1, const git_oid *oid2);
	~GitOidPair();

	void setPair(const git_oid *oid1, const git_oid *oid2);

//	GitOidPair &operator=(const GitOidPair &o);
	bool operator<(const GitOidPair &o) const;
	bool operator==(const GitOidPair &o) const;

private:
	git_oid m_oid1;
	git_oid m_oid2;
};

class DetectorBase
{
public:
	DetectorBase();
	~DetectorBase();

	void detect();

	int nPairs() const;

	void dumpPairs(std::vector<GitOidPair> &dest) const;

protected:
	void addOidPair(const git_oid *oid1, const git_oid *oid2);
	virtual void doDetect() = 0;

private:
	GitOidPair *m_oidPairs;
	int m_maxPairs;
	int m_nPairs;
};

#endif // DETECTORBASE_H

