#include <string>

struct _DBusGConnection;
typedef struct _DBusGConnection DBusGConnection;

struct _DBusGProxy;
typedef struct _DBusGProxy DBusGProxy;

class DBusLokalizeEditor
{
public:
    DBusLokalizeEditor(DBusGProxy *proxy);
    ~DBusLokalizeEditor();

    void setEntriesFilteredOut(bool filteredOut);
    void setEntryFilteredOut(int entry, bool filteredOut);
    void gotoEntry(int entry);
    void addTemporaryEntryNote(int entry, const char *note);
    void clearTemporaryEntryNotes(int entry);
    void openSyncSource(const std::string& filename);

private:
    DBusGProxy *m_proxy;
};

//--------------------------------------

class DBusLokalizeInterface
{
public:
    DBusLokalizeInterface();
    ~DBusLokalizeInterface();

    void connect();
    void disconnect();

    int getPid();
    DBusLokalizeEditor *openFileInEditor(const std::string& filename);

protected:
    DBusGProxy *createProxy(const std::string& service, const char *path, const char *interface);
    DBusGProxy *createEditorProxy(int index);

private:
    static bool s_gTypeInit;

    std::string m_lokalizeInstance;
    DBusGConnection* m_connection;
    DBusGProxy* m_proxy;
};
