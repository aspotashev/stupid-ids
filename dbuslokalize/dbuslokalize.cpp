
// http://dbus.freedesktop.org/doc/dbus-tutorial.html

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <gettextpo-helper/gettextpo-helper.h>

#include "dbuslokalize.h"
#include <dbus/dbus-glib.h>

bool strstartswith(const char *str, const char *pattern)
{
	return strncmp(str, pattern, strlen(pattern)) == 0;
}

bool DBusLokalizeInterface::s_gTypeInit = false;

DBusLokalizeInterface::DBusLokalizeInterface()
{
	m_connection = NULL;
	m_proxy = NULL;
	m_lokalizeInstance = NULL;

	if (!s_gTypeInit)
	{
		g_type_init();
		s_gTypeInit = true;
	}
}

DBusLokalizeInterface::~DBusLokalizeInterface()
{
	disconnect();
}

void DBusLokalizeInterface::connect()
{
	if (m_connection)
		return;

	assert(m_connection == NULL);
	assert(m_proxy == NULL);
	assert(m_lokalizeInstance == NULL);

	GError *error = NULL;
	m_connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	assert(m_connection);

	DBusGProxy *dbus_proxy = createProxy(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	// Getting list of DBus-aware processes
	error = NULL;
	char **name_list;
	assert(dbus_g_proxy_call(
		dbus_proxy, "ListNames", &error, G_TYPE_INVALID,
		G_TYPE_STRV, &name_list, G_TYPE_INVALID));
	g_object_unref(dbus_proxy);

	// Find Lokalize instance
	for (int i = 0; name_list[i]; i ++)
		if (strstartswith(name_list[i], "org.kde.lokalize"))
		{
			m_lokalizeInstance = xstrdup(name_list[i]);
			break;
		}
	g_strfreev(name_list);

	// Proxy for Lokalize interface
	m_proxy = createProxy(m_lokalizeInstance, "/ThisIsWhatYouWant", "org.kde.Lokalize.MainWindow");
}

void DBusLokalizeInterface::disconnect()
{
	if (!m_connection)
		return;

	assert(m_connection);
	assert(m_proxy);
	assert(m_lokalizeInstance);

	if (m_lokalizeInstance)
	{
		delete [] m_lokalizeInstance;
		m_lokalizeInstance = NULL;
	}

	if (m_proxy)
	{
		g_object_unref(m_proxy);
		m_proxy = NULL;
	}

	if (m_connection)
	{
		// TODO: how to disconnect?
		m_connection = NULL;
	}
}

DBusGProxy *DBusLokalizeInterface::createProxy(const char *service, const char *path, const char *interface)
{
	return dbus_g_proxy_new_for_name(m_connection, service, path, interface);
}

DBusGProxy *DBusLokalizeInterface::createEditorProxy(int index)
{
	assert(index >= 0);

	char *path = new char[200];
	sprintf(path, "/ThisIsWhatYouWant/Editor/%d", index);
	DBusGProxy *proxy = createProxy(m_lokalizeInstance, path, "org.kde.Lokalize.Editor");
	delete [] path;

	return proxy;
}

int DBusLokalizeInterface::getPid()
{
	connect();

	GError *error = NULL;
	int pid = -1;
	assert(dbus_g_proxy_call(
		m_proxy, "pid", &error, G_TYPE_INVALID,
		G_TYPE_INT, &pid, G_TYPE_INVALID));

	return pid;
}

DBusLokalizeEditor *DBusLokalizeInterface::openFileInEditor(const char *filename)
{
	connect();

	GError *error = NULL;
	int index = -1;
	assert(dbus_g_proxy_call(
		m_proxy, "openFileInEditor", &error,
		G_TYPE_STRING, filename, G_TYPE_INVALID,
		G_TYPE_INT, &index, G_TYPE_INVALID));

	return new DBusLokalizeEditor(createEditorProxy(index));
}

//--------------------------------------

DBusLokalizeEditor::DBusLokalizeEditor(DBusGProxy *proxy)
{
	m_proxy = proxy;
}

DBusLokalizeEditor::~DBusLokalizeEditor()
{
	g_object_unref(m_proxy);
}

void DBusLokalizeEditor::setEntriesFilteredOut(bool filteredOut)
{
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "setEntriesFilteredOut", &error,
		G_TYPE_BOOLEAN, filteredOut ? TRUE : FALSE, G_TYPE_INVALID,
		G_TYPE_INVALID));
}

void DBusLokalizeEditor::setEntryFilteredOut(int entry, bool filteredOut)
{
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "setEntryFilteredOut", &error,
		G_TYPE_INT, entry, G_TYPE_BOOLEAN, filteredOut ? TRUE : FALSE, G_TYPE_INVALID,
		G_TYPE_INVALID));
}

void DBusLokalizeEditor::gotoEntry(int entry)
{
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "gotoEntry", &error,
		G_TYPE_INT, entry, G_TYPE_INVALID,
		G_TYPE_INVALID));
}

void DBusLokalizeEditor::addTemporaryEntryNote(int entry, const char *note)
{
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "addTemporaryEntryNote", &error,
		G_TYPE_INT, entry, G_TYPE_STRING, note, G_TYPE_INVALID,
		G_TYPE_INVALID));
}

void DBusLokalizeEditor::clearTemporaryEntryNotes(int entry)
{
#define LOKALIZE_CLEAR_TEMPORARY_NOTES 1

#ifdef LOKALIZE_CLEAR_TEMPORARY_NOTES
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "clearTemporaryEntryNotes", &error,
		G_TYPE_INT, entry, G_TYPE_INVALID,
		G_TYPE_INVALID));
#else
	fprintf(stderr, "Not calling D-Bus method clearTemporaryEntryNotes, because LOKALIZE_CLEAR_TEMPORARY_NOTES is disabled.");
#endif
}

void DBusLokalizeEditor::openSyncSource(const char *filename)
{
	GError *error = NULL;
	assert(dbus_g_proxy_call(
		m_proxy, "openSyncSource", &error,
		G_TYPE_STRING, filename, G_TYPE_INVALID,
		G_TYPE_INVALID));
}

