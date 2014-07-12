#include "iddiffchange.h"
#include <stdexcept>

IddiffChange::IddiffChange()
{
    m_reviewComment = nullptr;
}

void IddiffChange::clearReviewComment()
{
    m_reviewComment = nullptr;
}

bool IddiffChange::empty() const
{
    return emptyReview() && emptyAdded() && emptyRemoved();
}

bool IddiffChange::emptyRemoved() const
{
    return m_removedItems.empty();
}

bool IddiffChange::emptyAdded() const
{
    return m_addedItems.empty();
}

bool IddiffChange::emptyReview() const
{
    return m_reviewComment == nullptr;
}

// static
void IddiffChange::eraseItem(std::vector<IddiffMessage *> &list, const IddiffMessage *item)
{
    for (std::vector<IddiffMessage *>::iterator iter = list.begin(); iter != list.end(); iter ++)
        if (*iter == item) // comparison by pointer, not by translations!
        {
            list.erase(iter);
            return;
        }

    throw std::runtime_error("item not found");
}

void IddiffChange::eraseRemoved(const IddiffMessage *item)
{
    eraseItem(m_removedItems, item);
}

void IddiffChange::eraseAdded(const IddiffMessage *item)
{
    eraseItem(m_addedItems, item);
}
