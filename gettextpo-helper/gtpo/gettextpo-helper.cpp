#include "block-sha1/sha1.h"

#include "gettextpo-helper.h"
#include "mappedfile.h"
#include "translationcontent.h"
#include "filecontentfs.h"
#include "iddiff.h"
#include "messagegroup.h"
#include "exceptionmessageinfo.h"
#include "gettextparserexception.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <stdexcept>
#include <memory>
#include <iomanip>

static void xerror_handler(
    int severity,
    po_message_t message, const char *filename, size_t lineno,
    size_t column, int multiline_p, const char *message_text)
{
    throw GettextParserException(
        severity,
        ExceptionMessageInfo(filename, lineno, column, multiline_p, message_text));
}

static void xerror2_handler(
    int severity,
    po_message_t message1, const char *filename1, size_t lineno1,
    size_t column1, int multiline_p1, const char *message_text1,
    po_message_t message2, const char *filename2, size_t lineno2,
    size_t column2, int multiline_p2, const char *message_text2)
{
    if (!filename2) {
        // Do not construct std::string on NULL string
        filename2 = "";
    }

    throw GettextParserException(
        severity,
        ExceptionMessageInfo(filename1, lineno1, column1, multiline_p1, message_text1),
        ExceptionMessageInfo(filename2, lineno2, column2, multiline_p2, message_text2));
}

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_read(const char *filename)
{
    struct po_xerror_handler xerror_handlers;
    xerror_handlers.xerror = xerror_handler;
    xerror_handlers.xerror2 = xerror2_handler;

    po_file_t file = po_file_read(filename, &xerror_handlers);
    if (file == NULL)
    {
        printf("Cannot read .po/.pot file: %s\n", filename);
        assert(0);
    }

    // ensure that there is only one domain "messages"
    const char * const *domains = po_file_domains(file);
    assert(strcmp(domains[0], "messages") == 0); // domain is not called "messages"
    assert(domains[1] == NULL); // more than one domain

    return file;
}

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_write(po_file_t file, const char *filename)
{
    struct po_xerror_handler xerror_handlers;
    xerror_handlers.xerror = xerror_handler;
    xerror_handlers.xerror2 = xerror2_handler;

    po_file_t res = po_file_write(file, filename, &xerror_handlers);
    if (res == NULL)
    {
        printf("Cannot write .po/.pot file: %s\n", filename);
        assert(0);
    }

    return res;
}

po_file_t po_buffer_read(const char *buffer, size_t length)
{
    // Child process writes the contents of the buffer to a pipe.
    // The read end of the pipe is connected to 'stdin' (for 'po_file_read').

    int pipe_fd[2];
    assert(pipe(pipe_fd) == 0);

    int stdin_fd = dup(0); // preserve 'stdin'
    assert(stdin_fd >= 0);
    close(0);

    assert(dup2(pipe_fd[0], 0) >= 0); // connect pipe's read end to stdin
    close(pipe_fd[0]);

    // pipe_fd[1] -- write end
    // fd=0 -- read end
    pid_t pid = fork();
    if (pid < 0) // fork failed
    {
        assert(0);
    }
    else if (pid == 0) // child process
    {
        close(0);

        // TODO: report errors from "write()" to the parent process (using a buffer + semaphore?)
        assert(write(pipe_fd[1], buffer, length) == static_cast<ssize_t>(length));

        exit(0); // terminate child process
    }

    // In parent process now

    close(pipe_fd[1]);
    po_file_t file = po_file_read("-"); // read from stdin

    // Restore 'stdin'
    close(0);
    assert(dup2(stdin_fd, 0) >= 0);
    close(stdin_fd);

    // Kill zombie child process, check exit status
    int status = -1;
    assert(waitpid(pid, &status, 0) == pid);
    assert(status == 0);

    return file;
}

//----------------------- Dumping translation files ------------------------

// Transform string into a string of characters [0-9a-f] or "n" if str is NULL.
std::string wrap_string_hex(const char *str)
{
    if (!str) {
        return "n";
    }

    std::ostringstream ss;

    size_t len = strlen(str);
    for (size_t i = 0; i < len; i ++) {
        ss << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(static_cast<unsigned char>(str[i]));
    }

    return ss.str();
}

//----------------------- Calculation of template-part hash ------------------------

std::string calculate_tp_hash(const char *filename)
{
    std::unique_ptr<TranslationContent> content(new TranslationContent(new FileContentFs(filename)));
    GitOid tp_hash = content->getTpHash();
    if (tp_hash.isNull()) {
//         throw std::runtime_error("Failed to calculate tp_hash");
        return "";
    }

    return tp_hash.toString();
}

// Returns the number of messages in .pot (excluding the header)
int get_pot_length(const char *filename)
{
    po_file_t file = po_file_read(filename); // all checks and error reporting are done in po_file_read

    po_message_iterator_t iterator = po_message_iterator(file, "messages");
    po_message_t message; // in fact, this is a pointer

    // processing header (header is the first message)
    message = po_next_message(iterator);
    assert(*po_message_msgid(message) == '\0'); // msgid of header must be empty

    int length = 0;
    // ordinary .po messages (not header)
    while ((message = po_next_message(iterator)))
        if (!po_message_is_obsolete(message)) // in fact, if we're processing a .pot, there can't be any obsolete messages
            length ++;

    // free memory
    po_message_iterator_free(iterator);
    po_file_free(file);

    return length;
}

//-------- Coupling IDs of equal messages in different .po/.pot files -------

struct MessageGroupMsgidCompare
{
    bool operator() (const MessageGroup* a, const MessageGroup* b) const
    {
        return a->compareByMsgid(*b);
    }
};

std::vector<std::vector<int> > list_equal_messages_ids(std::vector<std::pair<TranslationContent *, int> > files)
{
    std::vector<std::vector<int> > list;

    typedef std::map<MessageGroup *, std::vector<int>, MessageGroupMsgidCompare > msg_ids_map_t;
    msg_ids_map_t msg_ids;
    for (size_t d = 0; d < files.size(); d ++)
    {
        TranslationContent *content = files[d].first;
        int first_id = files[d].second;

        std::vector<MessageGroup *> messages = content->readMessages();
        for (size_t i = 0; i < messages.size(); i ++)
            msg_ids[messages[i]].push_back(first_id + i);
    }

    for (msg_ids_map_t::iterator iter = msg_ids.begin(); iter != msg_ids.end(); iter ++)
    {
        size_t n_ids = iter->second.size();
        if (n_ids == 0) // impossible
        {
            assert(0);
        }
        else if (n_ids >= 2)
        {
            // This 'assert' can be removed if files.size() > 2 (not doing this now,
            // because I do not know where a 3-way dump-ids may be applied).
            assert(n_ids == 2);

            list.push_back(iter->second); // adding a vector of all IDs of this message
        }
    }

    return list;
}

std::vector<std::pair<int, int> > list_equal_messages_ids_2(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b)
{
    std::vector<std::pair<TranslationContent *, int> > files;
    files.push_back(std::pair<TranslationContent *, int>(file_a, first_id_a));
    files.push_back(std::pair<TranslationContent *, int>(file_b, first_id_b));

    std::vector<std::vector<int> > list = list_equal_messages_ids(files);

    std::vector<std::pair<int, int> > res;
    for (std::vector<std::vector<int> >::iterator iter = list.begin(); iter != list.end(); iter ++)
    {
        // No more than 2 IDs denoting the same message can exists in 2 files
        assert(iter->size() == 2);

        res.push_back(std::pair<int, int>((*iter)[0], (*iter)[1]));
    }

    return res;
}

int dump_equal_messages_to_mmapdb(TranslationContent *file_a, int first_id_a, TranslationContent *file_b, int first_id_b, MappedFileIdMapDb *mmap_db)
{
    std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
        file_a, first_id_a, file_b, first_id_b);
    for (std::vector<std::pair<int, int> >::iterator iter = list.begin(); iter != list.end(); iter ++)
        mmap_db->addRow(iter->first, iter->second);

    return (int)list.size();
}

//------ For diff'ing tools ------

// Returns 'true' when msgstr (or all msgstr[i]) are empty.
bool po_message_is_untranslated(po_message_t message)
{
    if (po_message_msgstr_plural(message, 0)) // message with plurals
    {
        for (int i = 0; po_message_msgstr_plural(message, i); i ++)
            if (po_message_msgstr_plural(message, i)[0] != '\0') // non-empty string found
                return false; // not untranslated (i.e. translated or fuzzy)

        return true; // all strings are empty
    }
    else // message without plurals
    {
        return po_message_msgstr(message)[0] == '\0';
    }
}

// Returns 0 when the message does not use plural forms (i.e. no "msgid_plural")
int po_message_n_plurals(po_message_t message)
{
    int i;
    for (i = 0; po_message_msgstr_plural(message, i); i ++);

    return i;
}

// Returns 0 when msgstr (or all msgstr[i]) are the same in two messages.
int compare_po_message_msgstr(po_message_t message_a, po_message_t message_b)
{
    int n_plurals = po_message_n_plurals(message_a);
    assert(n_plurals == po_message_n_plurals(message_b));

    if (n_plurals > 0) // messages with plurals
    {
        for (int i = 0; i < n_plurals; i ++)
            if (strcmp(
                po_message_msgstr_plural(message_a, i),
                po_message_msgstr_plural(message_b, i)))
                return 1; // differences found

        return 0; // all strings are equal
    }
    else // message without plurals
    {
        return strcmp(po_message_msgstr(message_a), po_message_msgstr(message_b));
    }
}

std::string path_to_basename(std::string path)
{
    size_t pos = path.rfind('/');
    if (pos == std::string::npos) // no '/' in the path
        return path;
    else
        return path.substr(pos + 1);
}

std::vector<std::string> parseArgs(int argc, char *argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);

    return args;
}
