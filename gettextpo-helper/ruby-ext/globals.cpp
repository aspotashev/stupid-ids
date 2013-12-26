
#include <gettextpo-helper/gettextpo-helper.h>
#include <gettextpo-helper/detector.h>
#include <gettextpo-helper/message.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/config.h>
#include <gettextpo-helper/oidmapcache.h>

#include "module.h"
#include "message.h"
#include "translationcontent.h"
#include "idmapdb.h"

// Out-of-class methods

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

		assert(git_oid_fromstr(&oid1, oid1_str) == GIT_SUCCESS);
		assert(git_oid_fromstr(&oid2, oid2_str) == GIT_SUCCESS);

		// TODO: "git_oid_fmtraw" needed (for full portability)

		assert(sizeof(oid1) == GIT_OID_RAWSZ);
		assert(fwrite(&oid1, GIT_OID_RAWSZ, 1, f) == 1);
		assert(fwrite(&oid2, GIT_OID_RAWSZ, 1, f) == 1);
	}

	fclose(f);

	return Qnil;
}

// TODO: provide options[:load_obsolete] option
VALUE wrap_read_po_file_messages(VALUE self, VALUE filename)
{
	std::vector<MessageGroup *> messages = read_po_file_messages(StringValuePtr(filename), false);

	VALUE res = rb_ary_new();
	for (size_t i = 0; i < messages.size(); i ++)
		rb_ary_push(res, cMessage_wrap(messages[i]->message(0)));

	return res;
}

//------- stupids_conf --------

VALUE wrap_stupids_conf(VALUE self, VALUE key)
{
	return rb_str_new2(StupidsConf(StringValuePtr(key)).c_str());
}

VALUE wrap_stupids_conf_path(VALUE self, VALUE key)
{
	return rb_str_new2(expand_path(StupidsConf(StringValuePtr(key))).c_str());
}

//----------------------------

VALUE wrap_tphash_cache(VALUE self, VALUE oid_str)
{
	GitOid oid(StringValuePtr(oid_str));
	const git_oid *tp_hash_raw = TphashCache.getValue(oid.oid());
	if (tp_hash_raw)
		return rb_str_new2(GitOid(tp_hash_raw).toString().c_str());
	else
		return Qnil;
}


// init
void init_globals()
{
	rb_define_singleton_method(GettextpoHelper, "calculate_tp_hash", RUBY_METHOD_FUNC(wrap_calculate_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "get_pot_length", RUBY_METHOD_FUNC(wrap_get_pot_length), 1);
	rb_define_singleton_method(GettextpoHelper, "dump_equal_messages_to_mmapdb", RUBY_METHOD_FUNC(wrap_dump_equal_messages_to_mmapdb), 5);
//	rb_define_singleton_method(GettextpoHelper, "get_min_ids_by_tp_hash", RUBY_METHOD_FUNC(wrap_get_min_ids_by_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "detect_transitions_inc", RUBY_METHOD_FUNC(wrap_detect_transitions_inc), 4);
	rb_define_singleton_method(GettextpoHelper, "append_processed_pairs", RUBY_METHOD_FUNC(wrap_append_processed_pairs), 2);
	rb_define_singleton_method(GettextpoHelper, "read_po_file_messages", RUBY_METHOD_FUNC(wrap_read_po_file_messages), 1);
	rb_define_singleton_method(GettextpoHelper, "stupids_conf", RUBY_METHOD_FUNC(wrap_stupids_conf), 1);
	rb_define_singleton_method(GettextpoHelper, "stupids_conf_path", RUBY_METHOD_FUNC(wrap_stupids_conf_path), 1);
	rb_define_singleton_method(GettextpoHelper, "tphash_cache", RUBY_METHOD_FUNC(wrap_tphash_cache), 1);
}

