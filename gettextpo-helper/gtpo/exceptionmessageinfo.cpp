#include "exceptionmessageinfo.h"

#include <sstream>

ExceptionMessageInfo::ExceptionMessageInfo(
    const std::string& filename, size_t lineno, size_t column,
    bool multilineP, const std::string& messageText):
    m_filename(filename), m_lineno(lineno), m_column(column),
    m_multilineP(multilineP), m_messageText(messageText)
{
}

ExceptionMessageInfo::ExceptionMessageInfo():
    m_filename(), m_lineno(0), m_column(0),
    m_multilineP(false), m_messageText("<none>")
{
}

std::string ExceptionMessageInfo::toString() const
{
    std::stringstream ss;
    ss << "at " << m_filename <<
        "[L" << m_lineno << ":C" << m_column << "] " <<
        (m_multilineP ? "<multiline>" : "<single line>") <<
        " Error: " << m_messageText;

    return ss.str();
}
