#ifndef EXCEPTIONMESSAGEINFO_H
#define EXCEPTIONMESSAGEINFO_H

#include <string>

class ExceptionMessageInfo
{
public:
    ExceptionMessageInfo();
    ExceptionMessageInfo(
        const std::string& filename, size_t lineno, size_t column,
        bool multilineP, const std::string& messageText);

    std::string toString() const;

private:
    std::string m_filename;
    size_t m_lineno;
    size_t m_column;
    bool m_multilineP;
    std::string m_messageText;
};

#endif // EXCEPTIONMESSAGEINFO_H
