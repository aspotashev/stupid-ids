
void xerror_handler(
	int severity,
	po_message_t message, const char *filename, size_t lineno,
	size_t column, int multiline_p, const char *message_text)
{
	printf("filename = %s, lineno = %lu, column = %lu\n", filename, lineno, column);
	assert(0);
}

void xerror2_handler(
	int severity,
	po_message_t message1, const char *filename1, size_t lineno1,
	size_t column1, int multiline_p1, const char *message_text1,
	po_message_t message2, const char *filename2, size_t lineno2,
	size_t column2, int multiline_p2, const char *message_text2)
{
	assert(0);
}

// overloaded function, without 'xerror_handlers' argument
po_file_t po_file_read(const char *filename)
{
	struct po_xerror_handler xerror_handlers;
	xerror_handlers.xerror = xerror_handler;
	xerror_handlers.xerror2 = xerror2_handler;

	po_file_t file = po_file_read(filename, &xerror_handlers);
	if (file == NULL)
	{
		printf("Cannot read .po/.pot file: %s\n", filename);
		assert(0);
	}

	// ensure that there is only one domain "messages"
	const char * const *domains = po_file_domains(file);
	assert(strcmp(domains[0], "messages") == 0); // domain is not called "messages"
	assert(domains[1] == NULL); // more than one domain

	return file;
}

