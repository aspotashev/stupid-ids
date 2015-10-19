#ifndef GETTEXTPARSEREXCEPTION_H
#define GETTEXTPARSEREXCEPTION_H

#include "exceptionmessageinfo.h"

#include <exception>

class GettextParserException : public std::exception
{
public:
    GettextParserException(int severity, ExceptionMessageInfo msg1);
    GettextParserException(int severity, ExceptionMessageInfo msg1, ExceptionMessageInfo msg2);
    virtual ~GettextParserException() noexcept;

    virtual const char* what() const noexcept;

private:
    void buildWhatString();

    int m_severity;

    bool m_twoMessages;
    ExceptionMessageInfo m_msg1;
    ExceptionMessageInfo m_msg2;

    std::string m_what;
};

#endif // GETTEXTPARSEREXCEPTION_H
