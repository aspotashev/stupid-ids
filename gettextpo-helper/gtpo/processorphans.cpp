
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "processorphans.h"
#include "gettextpo-helper.h"

#include <stdexcept>

ProcessOrphansTxtEntry::ProcessOrphansTxtEntry(const char *cmd, const char *origin, const char *destination)
{
    if (!strcmp(cmd, "move"))
        m_type = MOVE;
    else if (!strcmp(cmd, "copy"))
        m_type = COPY;
    else if (!strcmp(cmd, "delete"))
        m_type = DELETE;
    else if (!strcmp(cmd, "merge"))
        m_type = MERGE;
    else if (!strcmp(cmd, "mergekeep"))
        m_type = MERGEKEEP;
    else
        throw std::runtime_error("unexpected command: " + std::string(cmd));


    assert(origin);

    // If command is not "delete", destination must be specified.
    //
    // If command is "delete", destination may
    // contain garbage (for example, SVN revision number).
    assert(m_type == DELETE || destination != NULL);


    m_origin = xstrdup(origin);
    splitFullnamePot(m_origin, &m_origPath, &m_origNamePot);

    m_destination = NULL;
    m_destPath = NULL;
    m_destNamePot = NULL;
    if (m_type != DELETE)
    {
        m_destination = xstrdup(destination);
        splitFullnamePot(m_destination, &m_destPath, &m_destNamePot);
    }
}

ProcessOrphansTxtEntry::~ProcessOrphansTxtEntry()
{
    delete [] m_origin;
    m_origin = NULL;

    delete [] m_origPath;
    m_origPath = NULL;

    delete [] m_origNamePot;
    m_origNamePot = NULL;

    if (m_type != DELETE)
    {
        delete [] m_destination;
        m_destination = NULL;

        delete [] m_destPath;
        m_destPath = NULL;

        delete [] m_destNamePot;
        m_destNamePot = NULL;
    }
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
void ProcessOrphansTxtEntry::splitFullnamePot(const char *fullname, char **path, char **name)
{
    const char *slash = strrchr(fullname, '/');
    int path_len = slash - fullname;

    *path = new char[path_len + 1]; // "+1" for '\0'
    memcpy(*path, fullname, path_len * sizeof(char));
    (*path)[path_len] = '\0';

    int name_len = (int)strlen(slash + 1);
    *name = new char[name_len + 2]; // "+2" for 't' and '\0'
    memcpy(*name, slash + 1, name_len * sizeof(char));
    (*name)[name_len] = 't'; // .po -> .pot
    (*name)[name_len + 1] = '\0';
}

const char *ProcessOrphansTxtEntry::destNamePot() const
{
    return m_destNamePot;
}

const char *ProcessOrphansTxtEntry::destPath() const
{
    return m_destPath;
}

const char *ProcessOrphansTxtEntry::origNamePot() const
{
    return m_origNamePot;
}

const char *ProcessOrphansTxtEntry::origPath() const
{
    return m_origPath;
}

//-------------------------------------------------------

ProcessOrphansTxt::ProcessOrphansTxt(const char *filename)
{
    FILE *f = fopen(filename, "r");
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
        m_entries.push_back(new ProcessOrphansTxtEntry(line, origin, destination));
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

void ProcessOrphansTxt::findByOrigin(std::vector<const ProcessOrphansTxtEntry *> &dest, const char *name, const char *path, int types = ProcessOrphansTxtEntry::ALL) const
{
    for (size_t i = 0; i < m_entries.size(); i ++)
        if ((m_entries[i]->type() & types) &&
            !strcmp(path, m_entries[i]->origPath()) &&
            !strcmp(name, m_entries[i]->origNamePot()))
        {
            dest.push_back(m_entries[i]);
        }
}

void ProcessOrphansTxt::findByDestination(std::vector<const ProcessOrphansTxtEntry *> &dest, const char *name, const char *path, int types = ProcessOrphansTxtEntry::ALL) const
{
    for (size_t i = 0; i < m_entries.size(); i ++)
        if ((m_entries[i]->type() & types) &&
            !strcmp(path, m_entries[i]->destPath()) &&
            !strcmp(name, m_entries[i]->destNamePot()))
        {
            dest.push_back(m_entries[i]);
        }
}

