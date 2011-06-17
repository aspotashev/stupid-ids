
#include <ruby.h>

#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/detector.h>
#include <gettextpo-helper/mappedfile.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/stupids-client.h>
#include <gettextpo-helper/gitloader.h>
#include <gettextpo-helper/message.h>

void init_search(const char *f_dump, const char *f_index, const char *f_map);
const char *find_string_id_by_str_multiple(char *s, int n);

MappedFileIdMapDb *rb_get_mapped_file(VALUE self);
TranslationContent *rb_get_translation_content(VALUE self);
GitLoader *rb_get_git_loader(VALUE self);

//-------------------------------

VALUE wrap_calculate_tp_hash(VALUE self, VALUE filename)
{
	std::string res = calculate_tp_hash(StringValuePtr(filename));
	return rb_str_new2(res.c_str());
}

VALUE wrap_get_pot_length(VALUE self, VALUE filename)
{
	return INT2FIX(get_pot_length(StringValuePtr(filename)));
}

VALUE wrap_dump_equal_messages_to_mmapdb(VALUE self, VALUE file_a, VALUE first_id_a, VALUE file_b, VALUE first_id_b, VALUE mmapdb)
{
	return INT2FIX(dump_equal_messages_to_mmapdb(
		rb_get_translation_content(file_a),
		FIX2INT(first_id_a),
		rb_get_translation_content(file_b),
		FIX2INT(first_id_b),
		rb_get_mapped_file(mmapdb)));
}

/*VALUE wrap_get_min_ids_by_tp_hash(VALUE self, VALUE tp_hash)
{
	std::vector<int> vector_res;

	try
	{
		vector_res = get_min_ids_by_tp_hash(StringValuePtr(tp_hash));
	}
	catch (TpHashNotFoundException &e)
	{
		return Qnil;
	}

	VALUE res = rb_ary_new();
	for (size_t i = 0; i < vector_res.size(); i ++)
		rb_ary_push(res, INT2FIX(vector_res[i]));

	return res;
}*/

VALUE rb_git_oid_fmt(const git_oid *oid)
{
	VALUE str = rb_str_buf_new(GIT_OID_HEXSZ); // TODO: how to avoid allocation before resizing? May be use str_alloc?
	rb_str_resize(str, GIT_OID_HEXSZ); // rb_str_resize also adds '\0' at the end
	char *str_ptr = StringValuePtr(str);
	git_oid_fmt(str_ptr, oid);

	return str;
}

VALUE wrap_detect_transitions_inc(VALUE self, VALUE path_trunk, VALUE path_stable, VALUE path_proorph, VALUE file_processed)
{
	printf("Detecting...\n");
	std::vector<GitOidPair> allPairs;
	detectTransitions(allPairs,
		StringValuePtr(path_trunk),
		StringValuePtr(path_stable),
		StringValuePtr(path_proorph));

	printf("Filtering...\n");
	std::vector<GitOidPair> pairs;
	filterProcessedTransitions(StringValuePtr(file_processed), allPairs, pairs);

	printf("Returning into Ruby...\n");
	VALUE res = rb_ary_new(); // create array
	for (size_t i = 0; i < pairs.size(); i ++)
	{
		VALUE pair = rb_ary_new(); // create array for next pair
		rb_ary_push(pair, rb_git_oid_fmt(pairs[i].oid1()));
		rb_ary_push(pair, rb_git_oid_fmt(pairs[i].oid2()));

		rb_ary_push(res, pair);
	}
	printf("detect_transitions_inc finished.\n\n");

	return res;
}

VALUE wrap_append_processed_pairs(VALUE self, VALUE file_processed, VALUE pairs)
{
	FILE *f = fopen(StringValuePtr(file_processed), "ab");
	assert(f);

	long len = RARRAY_LEN(pairs);
	const char *oid1_str;
	const char *oid2_str;
	git_oid oid1;
	git_oid oid2;
	unsigned char oid1_raw[GIT_OID_RAWSZ];
	unsigned char oid2_raw[GIT_OID_RAWSZ];
	for (long offset = 0; offset < len; offset ++)
	{
		VALUE pair = rb_ary_entry(pairs, offset);
		VALUE p0 = rb_ary_entry(pair, 0);
		VALUE p1 = rb_ary_entry(pair, 1);

		oid1_str = StringValuePtr(p0);
		oid2_str = StringValuePtr(p1);

		assert(git_oid_mkstr(&oid1, oid1_str) == GIT_SUCCESS);
		assert(git_oid_mkstr(&oid2, oid2_str) == GIT_SUCCESS);

		// TODO: "git_oid_fmtraw" needed (for full portability)

		assert(sizeof(oid1) == GIT_OID_RAWSZ);
		assert(fwrite(&oid1, GIT_OID_RAWSZ, 1, f) == 1);
		assert(fwrite(&oid2, GIT_OID_RAWSZ, 1, f) == 1);
	}

	fclose(f);

	return Qnil;
}

//------- class GettextpoHelper::GitLoader -------

void free_cGitLoader(void *content)
{
	delete (GitLoader *)content;
}

VALUE cGitLoader;

VALUE cGitLoader_new(VALUE klass)
{
	GitLoader *git_loader = new GitLoader();
	return Data_Wrap_Struct(cGitLoader, NULL, free_cGitLoader, git_loader);
}

VALUE cGitLoader_add_repository(VALUE self, VALUE git_dir)
{
	GitLoader *git_loader = rb_get_git_loader(self);
	git_loader->addRepository(StringValuePtr(git_dir));

	return Qnil;
}


GitLoader *rb_get_git_loader(VALUE self)
{
	GitLoader *git_loader;
	Data_Get_Struct(self, GitLoader, git_loader);
	assert(git_loader);

	return git_loader;
}

//------- class GettextpoHelper::TranslationContent -------

void free_cTranslationContent(void *content)
{
	delete (TranslationContent *)content;
}

VALUE cTranslationContent;

VALUE cTranslationContent_new_file(VALUE klass, VALUE filename)
{
	TranslationContent *content = new TranslationContent(StringValuePtr(filename));
	return Data_Wrap_Struct(cTranslationContent, NULL, free_cTranslationContent, content);
}

VALUE cTranslationContent_new_git(VALUE klass, VALUE git_loader, VALUE oid_str)
{
	git_oid oid;
	assert(git_oid_mkstr(&oid, StringValuePtr(oid_str)) == GIT_SUCCESS);

	TranslationContent *content = new TranslationContent(rb_get_git_loader(git_loader), &oid);
	return Data_Wrap_Struct(cTranslationContent, NULL, free_cTranslationContent, content);
}

TranslationContent *rb_get_translation_content(VALUE self)
{
	TranslationContent *content;
	Data_Get_Struct(self, TranslationContent, content);
	assert(content);

	return content;
}

//------- class GettextpoHelper::IdMapDb -------

void free_cIdMapDb(void *mapped_file)
{
	delete (MappedFileIdMapDb *)mapped_file;
}

VALUE cIdMapDb;

VALUE cIdMapDb_new(VALUE klass, VALUE filename)
{
	MappedFileIdMapDb *mapped_file = new MappedFileIdMapDb(StringValuePtr(filename));
	return Data_Wrap_Struct(cIdMapDb, NULL, free_cIdMapDb, mapped_file);
}

MappedFileIdMapDb *rb_get_mapped_file(VALUE self)
{
	MappedFileIdMapDb *mapped_file;
	Data_Get_Struct(self, MappedFileIdMapDb, mapped_file);
	assert(mapped_file);

	return mapped_file;
}

VALUE cIdMapDb_create(VALUE self, VALUE ary)
{
	MappedFileIdMapDb *mapped_file = rb_get_mapped_file(self);

	long len = RARRAY_LEN(ary);
	for (long offset = 0; offset < len; offset ++)
	{
		VALUE item = rb_ary_entry(ary, offset);

		int msg_id = FIX2INT(rb_ary_entry(item, 0));
		int min_id = FIX2INT(rb_ary_entry(item, 1));

		mapped_file->addRow(msg_id, min_id);
	}

	return Qnil;
}

VALUE cIdMapDb_normalize_database(VALUE self)
{
	MappedFileIdMapDb *mapped_file = rb_get_mapped_file(self);
	mapped_file->normalizeDatabase();

	return Qnil;
}

VALUE cIdMapDb_get_min_id(VALUE self, VALUE msg_id)
{
	MappedFileIdMapDb *mapped_file = rb_get_mapped_file(self);
	return INT2FIX(mapped_file->getRecursiveMinId(FIX2INT(msg_id)));
}

VALUE cIdMapDb_get_min_id_array(VALUE self, VALUE first_msg_id, VALUE num)
{
	MappedFileIdMapDb *mapped_file = rb_get_mapped_file(self);
	std::vector<int> res_int = mapped_file->getMinIdArray(FIX2INT(first_msg_id), FIX2INT(num));

	VALUE res = rb_ary_new();
	for (int i = 0; i < FIX2INT(num); i ++)
		rb_ary_push(res, INT2FIX(res_int[i]));

	return res;
}

//------- class GettextpoHelper::Message -------

void free_cMessage(void *message)
{
	delete (Message *)message;
}

VALUE cMessage;

VALUE cMessage_wrap(Message *message)
{
	return Data_Wrap_Struct(cMessage, NULL, free_cMessage, message);
}

Message *rb_get_message(VALUE self)
{
	Message *message;
	Data_Get_Struct(self, Message, message);
	assert(message);

	return message;
}

VALUE cMessage_num_plurals(VALUE self)
{
	Message *message = rb_get_message(self);
	return INT2FIX(message->numPlurals());
}

VALUE cMessage_equal_translations(VALUE self, VALUE other)
{
	return rb_get_message(self)->equalTranslationsComments(rb_get_message(other)) ? Qtrue : Qfalse;
}

VALUE cMessage_is_fuzzy(VALUE self)
{
	return rb_get_message(self)->isFuzzy() ? Qtrue : Qfalse;
}

VALUE cMessage_is_plural(VALUE self)
{
	return rb_get_message(self)->isPlural() ? Qtrue : Qfalse;
}

VALUE cMessage_is_untranslated(VALUE self)
{
	return rb_get_message(self)->isUntranslated() ? Qtrue : Qfalse;
}

VALUE cMessage_msgstr(VALUE self, VALUE plural_form)
{
	return rb_str_new2(rb_get_message(self)->msgstr(FIX2INT(plural_form)));
}

VALUE cMessage_msgcomments(VALUE self)
{
	return rb_str_new2(rb_get_message(self)->msgcomments());
}

//------- read_po_file_messages --------

// TODO: provide options[:load_obsolete] option
VALUE wrap_read_po_file_messages(VALUE self, VALUE filename)
{
	std::vector<MessageGroup *> messages = read_po_file_messages(StringValuePtr(filename), false);

	VALUE res = rb_ary_new();
	for (size_t i = 0; i < messages.size(); i ++)
		rb_ary_push(res, cMessage_wrap(messages[i]->message(0)));

	return res;
}

extern "C" {

/* Function called at module loading */
void Init_stupidsruby()
{
	VALUE GettextpoHelper = rb_define_module("GettextpoHelper");
	rb_define_singleton_method(GettextpoHelper, "calculate_tp_hash", RUBY_METHOD_FUNC(wrap_calculate_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "get_pot_length", RUBY_METHOD_FUNC(wrap_get_pot_length), 1);
	rb_define_singleton_method(GettextpoHelper, "dump_equal_messages_to_mmapdb", RUBY_METHOD_FUNC(wrap_dump_equal_messages_to_mmapdb), 5);
//	rb_define_singleton_method(GettextpoHelper, "get_min_ids_by_tp_hash", RUBY_METHOD_FUNC(wrap_get_min_ids_by_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "detect_transitions_inc", RUBY_METHOD_FUNC(wrap_detect_transitions_inc), 4);
	rb_define_singleton_method(GettextpoHelper, "append_processed_pairs", RUBY_METHOD_FUNC(wrap_append_processed_pairs), 2);

	cGitLoader = rb_define_class_under(GettextpoHelper, "GitLoader", rb_cObject);
	rb_define_singleton_method(cGitLoader, "new", RUBY_METHOD_FUNC(cGitLoader_new), 0);
	rb_define_method(cGitLoader, "add_repository", RUBY_METHOD_FUNC(cGitLoader_add_repository), 1);

	cTranslationContent = rb_define_class_under(GettextpoHelper, "TranslationContent", rb_cObject);
	rb_define_singleton_method(cTranslationContent, "new", RUBY_METHOD_FUNC(cTranslationContent_new_file), 1);
	rb_define_singleton_method(cTranslationContent, "new", RUBY_METHOD_FUNC(cTranslationContent_new_git), 2);

	cIdMapDb = rb_define_class_under(GettextpoHelper, "IdMapDb", rb_cObject);
	rb_define_singleton_method(cIdMapDb, "new", RUBY_METHOD_FUNC(cIdMapDb_new), 1);
	rb_define_method(cIdMapDb, "create", RUBY_METHOD_FUNC(cIdMapDb_create), 1);
	rb_define_method(cIdMapDb, "normalize_database", RUBY_METHOD_FUNC(cIdMapDb_normalize_database), 0);
	rb_define_method(cIdMapDb, "get_min_id", RUBY_METHOD_FUNC(cIdMapDb_get_min_id), 1);
	rb_define_method(cIdMapDb, "get_min_id_array", RUBY_METHOD_FUNC(cIdMapDb_get_min_id_array), 2);

	rb_define_singleton_method(GettextpoHelper, "read_po_file_messages", RUBY_METHOD_FUNC(wrap_read_po_file_messages), 1);

	cMessage = rb_define_class_under(GettextpoHelper, "Message", rb_cObject);
	rb_define_method(cMessage, "num_plurals", RUBY_METHOD_FUNC(cMessage_num_plurals), 0);
	rb_define_method(cMessage, "equal_translations", RUBY_METHOD_FUNC(cMessage_equal_translations), 1);
	rb_define_method(cMessage, "is_fuzzy", RUBY_METHOD_FUNC(cMessage_is_fuzzy), 0);
	rb_define_method(cMessage, "is_plural", RUBY_METHOD_FUNC(cMessage_is_plural), 0);
	rb_define_method(cMessage, "is_untranslated", RUBY_METHOD_FUNC(cMessage_is_untranslated), 0);
	rb_define_method(cMessage, "msgstr", RUBY_METHOD_FUNC(cMessage_msgstr), 1);
	rb_define_method(cMessage, "msgcomments", RUBY_METHOD_FUNC(cMessage_msgcomments), 0);
}

}

