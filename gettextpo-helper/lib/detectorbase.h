
#ifndef DETECTORBASE_H
#define DETECTORBASE_H

#include <vector>

#include <git2.h>

class GitOid
{
public:
	GitOid();
	GitOid(const git_oid *oid);
	GitOid(const char *oid_str);
	~GitOid();

	void setOid(const git_oid *oid);
	void setOidStr(const char *oid_str);
	void setOidRaw(const unsigned char *oid_raw);

	bool operator<(const GitOid &o) const;
	bool operator==(const GitOid &o) const;

	const git_oid *oid() const;

private:
	git_oid m_oid;
};

class CommitFileChange;

class GitOid2Change : public GitOid
{
public:
	GitOid2Change();
	GitOid2Change(const git_oid *oid, const CommitFileChange *change);
	~GitOid2Change();

	const CommitFileChange *change() const;

private:
	const CommitFileChange *m_change;
};

//----------------------------------------

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

	const git_oid *oid1() const;
	const git_oid *oid2() const;

private:
	git_oid m_oid1;
	git_oid m_oid2;
};

//----------------------------------------

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

