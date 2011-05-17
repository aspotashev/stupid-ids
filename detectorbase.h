
#ifndef DETECTORBASE_H
#define DETECTORBASE_H

#include <git2.h>

class GitOidPair
{
public:
	GitOidPair(const git_oid *oid1, const git_oid *oid2);
	~GitOidPair();

	void setPair(const git_oid *oid1, const git_oid *oid2);

//	GitOidPair &operator=(const GitOidPair &o);

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

protected:
	void addOidPair(const git_oid *oid1, const git_oid *oid2);
	virtual void doDetect() = 0;

private:
	GitOidPair *m_oidPairs;
	int m_maxPairs;
	int m_nPairs;
};

#endif // DETECTORBASE_H

