#ifndef IDDIFFCHANGE_H
#define IDDIFFCHANGE_H

#include <vector>

class IddiffMessage;

class IddiffChange
{
public:
    IddiffChange();

    bool empty() const;
    bool emptyRemoved() const;
    bool emptyAdded() const;
    bool emptyReview() const;
    void clearReviewComment();

    static void eraseItem(std::vector<IddiffMessage*>& list, const IddiffMessage* item);
    void eraseRemoved(const IddiffMessage* item);
    void eraseAdded(const IddiffMessage* item);

//private:
    std::vector<IddiffMessage*> m_removedItems;
    std::vector<IddiffMessage*> m_addedItems;
    IddiffMessage *m_reviewComment;
};

#endif // IDDIFFCHANGE_H
