
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "processorphans.h"
#include "gettextpo-helper.h"

#include <stdexcept>

ProcessOrphansTxtEntry::ProcessOrphansTxtEntry(
    const std::string& cmd, const std::string& origin, const OptString& destination)
{
    if (cmd == "move")
        m_type = MOVE;
    else if (cmd == "copy")
        m_type = COPY;
    else if (cmd == "delete")
        m_type = DELETE;
    else if (cmd == "merge")
        m_type = MERGE;
    else if (cmd == "mergekeep")
        m_type = MERGEKEEP;
    else
        throw std::runtime_error("unexpected command: " + std::string(cmd));

    // If command is not "delete", destination must be specified.
    //
    // If command is "delete", destination may
    // contain garbage (for example, SVN revision number).
    assert(m_type == DELETE || !destination.isNull());

    m_origin = origin;
    std::tie(m_origPath, m_origNamePot) = splitFullnamePot(m_origin);

    m_destination = destination;
    std::tie(m_destPath, m_destNamePot) = splitFullnamePot(m_destination);
}

ProcessOrphansTxtEntry::~ProcessOrphansTxtEntry()
{
}

int ProcessOrphansTxtEntry::type() const
{
    return m_type;
}

/*
const char *ProcessOrphansTxtEntry::origin() const
{
    assert(m_origin);

    return m_origin;
}

const char *ProcessOrphansTxtEntry::destination() const
{
    assert(m_destination);

    return m_destination;
}
*/

/*
void ProcessOrphansTxtEntry::splitFullname(const char *fullname, char **path, char **name)
{
    const char *slash = strrchr(fullname, '/');
    int path_len = slash - fullname;

    *path = new char[path_len + 1];
    memcpy(*path, fullname, path_len * sizeof(char));
    (*path)[path_len] = '\0';

    *name = xstrdup(slash + 1);
}
*/

// Adds letter "t" to the end of file name
//
// static
std::pair<std::string, std::string> ProcessOrphansTxtEntry::splitFullnamePot(
    const std::string& fullname)
{
    size_t slashPos = fullname.rfind('/');
    if (slashPos == std::string::npos)
        throw std::runtime_error("Not slash found in the whole path to file");

    // Make change in basename: .po -> .pot
    return std::make_pair(
        fullname.substr(0, slashPos),
        fullname.substr(slashPos + 1) + std::string("t"));
}

std::string ProcessOrphansTxtEntry::destNamePot() const
{
    return m_destNamePot;
}

std::string ProcessOrphansTxtEntry::destPath() const
{
    return m_destPath;
}

std::string ProcessOrphansTxtEntry::origNamePot() const
{
    return m_origNamePot;
}

std::string ProcessOrphansTxtEntry::origPath() const
{
    return m_origPath;
}

//-------------------------------------------------------

ProcessOrphansTxt::ProcessOrphansTxt(const std::string& filename)
{
    FILE *f = fopen(filename.c_str(), "r");
    if (!f)
        throw std::runtime_error("Could not open file " + std::string(filename));

    static char line[10000];
    while (1)
    {
        if (!fgets(line, 10000, f))
            break;

        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r' || !strcmp(line, " \n")) // empty line
            continue;
        if (line[0] == '#') // comment
            continue;

        char *origin = strchr(line, ' ');
        if (!origin)
        {
            printf("Space not found in string: [%s]\n", line);
            assert(0);
        }
        *origin = '\0'; // cut off the command (in 'line')
        origin ++; // moving to the beginning of the origin path

        char *destination = strchr(origin, ' ');
        if (!destination) // command "delete" does not require destination
        {
            origin[strcspn(origin, " \n\r")] = '\0';
        }
        else
        {
            *destination = '\0'; // cut off the origin path (in 'origin')
            destination ++; // moving to the beginning of the destination path

            destination[strcspn(destination, " \n\r")] = '\0';
        }

//      printf("cmd = [%s]\norigin = [%s]\ndest = [%s]\n\n", line, origin, destination);
        m_entries.push_back(new ProcessOrphansTxtEntry(line, origin, OptString(destination)));
    }
}

ProcessOrphansTxt::~ProcessOrphansTxt()
{
    for (size_t i = 0; i < m_entries.size(); i ++)
    {
        delete m_entries[i];
        m_entries[i] = NULL;
    }
}

void ProcessOrphansTxt::findByOrigin(
    std::vector<const ProcessOrphansTxtEntry*>& dest,
    const std::string& name, const std::string& path, int types) const
{
    for (size_t i = 0; i < m_entries.size(); i ++)
        if ((m_entries[i]->type() & types) &&
            path == m_entries[i]->origPath() &&
            name == m_entries[i]->origNamePot())
        {
            dest.push_back(m_entries[i]);
        }
}

void ProcessOrphansTxt::findByDestination(
    std::vector<const ProcessOrphansTxtEntry*> &dest,
    const std::string& name,
    const std::string& path,
    int types = ProcessOrphansTxtEntry::ALL) const
{
    for (size_t i = 0; i < m_entries.size(); i ++)
        if ((m_entries[i]->type() & types) &&
            path == m_entries[i]->destPath() &&
            name == m_entries[i]->destNamePot())
        {
            dest.push_back(m_entries[i]);
        }
}
