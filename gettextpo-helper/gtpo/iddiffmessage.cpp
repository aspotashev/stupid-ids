#include "iddiffmessage.h"
#include "message.h"

IddiffMessage::IddiffMessage():
    MessageTranslationBase()
{
}

IddiffMessage::IddiffMessage(const IddiffMessage& msg)
    : MessageTranslationBase(msg)
{
}

IddiffMessage::IddiffMessage(const Message& msg)
    : MessageTranslationBase(msg)
{
}

IddiffMessage::IddiffMessage(po_message_t message):
    MessageTranslationBase(message)
{
}

IddiffMessage::~IddiffMessage()
{
    // TODO: free memory
}

void IddiffMessage::addMsgstr(const OptString& str)
{
    m_msgstr.push_back(str);
}

void IddiffMessage::setFuzzy(bool fuzzy)
{
    m_fuzzy = fuzzy;
}

/**
 * Fuzzy flag state will not be copied.
 */
// TODO: how to replace this?
void IddiffMessage::copyTranslationsToMessage(Message* message) const
{
    assert(numPlurals() == message->numPlurals());

    for (int i = 0; i < numPlurals(); ++i)
        message->editMsgstr(i, msgstr(i));
}

bool IddiffMessage::isTranslated() const
{
    return !isFuzzy() && !isUntranslated();
}
