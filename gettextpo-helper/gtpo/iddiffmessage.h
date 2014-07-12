#ifndef IDDIFFMESSAGE_H
#define IDDIFFMESSAGE_H

#include "messagetranslationbase.h"

class Message;

class IddiffMessage : public MessageTranslationBase
{
public:
    /**
     * @brief Constructs an empty IddiffMessage.
     **/
    IddiffMessage();

    /**
     * @brief Copying constructor.
     *
     * All internal dynamically allocatable strings will also be duplicated.
     *
     * @param msg The message to copy from.
     **/
    IddiffMessage(const IddiffMessage &msg);

    IddiffMessage(const Message &msg);

    IddiffMessage(po_message_t message);
    virtual ~IddiffMessage();

    void setFuzzy(bool fuzzy);

    bool isTranslated() const;

    void addMsgstr(const OptString& str);

    void copyTranslationsToMessage(Message* message) const;
};

#endif // IDDIFFMESSAGE_H
