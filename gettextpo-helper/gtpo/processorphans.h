#include "optstring.h"

#include <vector>
#include <string>

class ProcessOrphansTxtEntry
{
public:
    enum
    {
        MOVE = 1,
        COPY = 2,
        DELETE = 4,
        MERGE = 8,
        MERGEKEEP = 16,
        ALL = MOVE | COPY | DELETE | MERGE | MERGEKEEP
    };

    ProcessOrphansTxtEntry(
        const std::string& cmd,
        const std::string& origin, const OptString& destination);
    ~ProcessOrphansTxtEntry();

    int type() const;

//  const char *origin() const;
    std::string origNamePot() const;
    std::string origPath() const;

//  const char *destination() const;
    std::string destNamePot() const;
    std::string destPath() const;

protected:
    // Returns pathname and basename
    static std::pair<std::string, std::string> splitFullnamePot(const std::string& fullname);

private:
    int m_type;
    std::string m_origin;
    std::string m_destination;

    std::string m_destPath;
    std::string m_destNamePot;
    std::string m_origPath;
    std::string m_origNamePot;
};

//-------------------------------------------------------

class ProcessOrphansTxt
{
public:
    ProcessOrphansTxt(const std::string& filename);
    ~ProcessOrphansTxt();

    void findByOrigin(
        std::vector<const ProcessOrphansTxtEntry *>& dest,
        const std::string& name,
        const std::string& path,
        int types) const;
    void findByDestination(
        std::vector<const ProcessOrphansTxtEntry*> &dest,
        const std::string& name,
        const std::string& path,
        int types) const;

private:
    std::vector<ProcessOrphansTxtEntry *> m_entries;
};

