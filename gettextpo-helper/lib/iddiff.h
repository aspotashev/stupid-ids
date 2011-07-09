
#include <string>
#include <vector>
#include <sstream>
#include <map>

#include <gettext-po.h>

#include <gettextpo-helper/message.h>
#include <gettextpo-helper/filedatetime.h>


class Message;
class MessageGroup;

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

	IddiffMessage(po_message_t message);
	~IddiffMessage();

	void setFuzzy(bool fuzzy);

	bool isTranslated() const;

	void addMsgstr(const char *str);

	void copyTranslationsToMessage(Message *message) const;
};

//----------------------------------------------

class TranslationContent;
class StupIdTranslationCollector;

class Iddiffer
{
public:
	Iddiffer();
	~Iddiffer();

	/**
	 * \brief Fills "ADDED" section with strings from the given TranslationContent
	 *
	 * "REMOVED" and "REVIEW" sections will be cleared.
	 * Old items from "ADDED" section will be deleted.
	 */
	void diffAgainstEmpty(TranslationContent *content_b);

	/**
	 * \brief Fills "ADDED" and "DELETED" sections according to differences between the two given TranslationContents
	 *
	 * "REVIEW" section will be cleared.
	 * Old items from "REMOVED" and "ADDED" sections will be deleted.
	 */
	void diffFiles(TranslationContent *content_a, TranslationContent *content_b);

	/**
	 * \brief Load iddiff from an .iddiff file
	 *
	 * It actually merges the given file with the changes already existing in the current Iddiffer object.
	 */
	bool loadIddiff(const char *filename);

	void writeToFile(const char *filename);

	/**
	 * \brief Replace all message IDs in the Iddiff with minimized IDs
	 */
	void minimizeIds();

	/**
	 * \brief Returns a _sorted_ vector of message IDs that exist at least in one section (ADDED, REMOVED or REVIEW).
	 */
	std::vector<int> involvedIds();

	/**
	 * \brief Formats the Iddiff as a text file.
	 *
	 * Returns empty string (even without headers) if the Iddiff is empty.
	 */
	std::string generateIddiffText();

	std::vector<IddiffMessage *> getIddiffArr(std::vector<std::pair<int, IddiffMessage *> > &section, int msg_id);

	bool canDropMessage(const Message *message, int min_id);
	void applyToMessage(MessageGroup *messageGroup, int min_id);
	void applyToContent(TranslationContent *content);
	void applyIddiff(StupIdTranslationCollector *collector);

	// low-level functions
	void clearIddiff();
	void clearReviewComments();
	void clearReviewComment(int msg_id);

	// low-level functions
	std::vector<IddiffMessage *> findRemoved(int msg_id);
	std::vector<IddiffMessage *> findAdded(int msg_id);
	IddiffMessage *findAddedSingle(int msg_id);
	IddiffMessage *findRemoved(int msg_id, const IddiffMessage *item);
	IddiffMessage *findAdded(int msg_id, const IddiffMessage *item);
	void eraseRemoved(int msg_id, const IddiffMessage *item);
	void eraseAdded(int msg_id, const IddiffMessage *item);

	const char *reviewCommentText(int msg_id);

	// These functions take ownership of "item"
	void insertRemoved(int msg_id, IddiffMessage *item);
	void insertAdded(int msg_id, IddiffMessage *item);
	void insertReview(int msg_id, IddiffMessage *item);
	void insertRemoved(std::pair<int, IddiffMessage *> item);
	void insertAdded(std::pair<int, IddiffMessage *> item);
	void insertReview(std::pair<int, IddiffMessage *> item);

	void insertRemovedClone(std::pair<int, IddiffMessage *> item);
	void insertAddedClone(std::pair<int, IddiffMessage *> item);
	void insertReviewClone(std::pair<int, IddiffMessage *> item);

	// low-level functions
	std::vector<std::pair<int, IddiffMessage *> > getRemovedVector();
	std::vector<std::pair<int, IddiffMessage *> > getAddedVector();
	std::vector<std::pair<int, IddiffMessage *> > getReviewVector();

	void mergeHeaders(Iddiffer *diff);
	void merge(Iddiffer *diff);

	void acceptTranslation(int msg_id, const IddiffMessage *item);
	void rejectTranslation(int msg_id, const IddiffMessage *item);

	bool isAcceptAlreadyReviewed(int msg_id, IddiffMessage *item);
	bool isRejectAlreadyReviewed(int msg_id, IddiffMessage *item);

	/**
	 * \brief Sets the "Date:" field of the Iddiff to the current date/time.
	 */
	void setCurrentDateTime();

private:
	void writeMessageList(std::vector<std::pair<int, IddiffMessage *> > list);
	std::pair<int, IddiffMessage *> loadMessageListEntry(const char *line);

	static IddiffMessage *findIddiffMessageList(std::vector<IddiffMessage *> list, const IddiffMessage *item);
	std::string dateString() const;
	const FileDateTime &date() const;

	// Helper functions for minimizeIds()
	template <typename T> static void substituteMsgId(std::map<int, T> &items, int old_id, int new_id);
	void substituteMsgId(int old_id, int new_id);

	// Helper function for getRemovedVector() and getAddedVector()
	static std::vector<std::pair<int, IddiffMessage *> > getItemsVector(std::map<int, std::vector<IddiffMessage *> > &items);

	// Helper function for eraseRemoved() and eraseAdded()
	static void eraseItem(std::map<int, std::vector<IddiffMessage *> > &items, int msg_id, const IddiffMessage *item);

private:
	std::string m_subject;
	std::string m_author;
	FileDateTime m_date;

	std::map<int, std::vector<IddiffMessage *> > m_removedItems;
	std::map<int, std::vector<IddiffMessage *> > m_addedItems;
	std::map<int, IddiffMessage *> m_reviewComments;
	bool m_minimizedIds;

	std::ostringstream m_output;
};

