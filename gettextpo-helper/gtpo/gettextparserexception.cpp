#include "gettextparserexception.h"

#include <sstream>

GettextParserException::GettextParserException(int severity, ExceptionMessageInfo msg1):
    std::exception(),
    m_severity(severity),
    m_twoMessages(false), m_msg1(msg1)
{
    buildWhatString();
}

GettextParserException::GettextParserException(int severity, ExceptionMessageInfo msg1, ExceptionMessageInfo msg2):
    std::exception(),
    m_severity(severity),
    m_twoMessages(true), m_msg1(msg1), m_msg2(msg2)
{
    buildWhatString();
}

GettextParserException::~GettextParserException() noexcept
{
}

void GettextParserException::buildWhatString()
{
    std::stringstream ss;
    ss << "GettextParserException: severity = " << m_severity << std::endl;
    ss << "    message 1: " << m_msg1.toString() << std::endl;
    if (m_twoMessages)
        ss << "    message 2: " << m_msg2.toString() << std::endl;

    m_what = ss.str();
}

const char* GettextParserException::what() const throw()
{
    return m_what.c_str();
}
