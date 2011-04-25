/* http://www.rubyinside.com/how-to-create-a-ruby-extension-in-c-in-under-5-minutes-100.html */

#include "ruby.h"

#include "../include/gettextpo-helper.h"
#include "mappedfile.h"

void init_search(const char *f_dump, const char *f_index, const char *f_map);
const char *find_string_id_by_str_multiple(char *s, int n);

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

VALUE wrap_list_equal_messages_ids_2(VALUE self, VALUE filename_a, VALUE first_id_a, VALUE filename_b, VALUE first_id_b)
{
	std::vector<std::pair<int, int> > list = list_equal_messages_ids_2(
		StringValuePtr(filename_a), FIX2INT(first_id_a),
		StringValuePtr(filename_b), FIX2INT(first_id_b));

	VALUE res = rb_ary_new(); // create array
	for (std::vector<std::pair<int, int> >::iterator iter = list.begin(); iter != list.end(); iter ++)
	{
		VALUE pair = rb_ary_new(); // create array for next pair
		rb_ary_push(pair, INT2FIX(iter->first));
		rb_ary_push(pair, INT2FIX(iter->second));

		rb_ary_push(res, pair);
	}

	return res;
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
		int merge_pair_id = FIX2INT(rb_ary_entry(item, 2));

		mapped_file->addRow(msg_id, min_id, merge_pair_id);
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

extern "C" {

/* Function called at module loading */
void Init_gettextpo_helper()
{
	VALUE GettextpoHelper = rb_define_module("GettextpoHelper");
	rb_define_singleton_method(GettextpoHelper, "calculate_tp_hash", RUBY_METHOD_FUNC(wrap_calculate_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "get_pot_length", RUBY_METHOD_FUNC(wrap_get_pot_length), 1);
	rb_define_singleton_method(GettextpoHelper, "list_equal_messages_ids_2", RUBY_METHOD_FUNC(wrap_list_equal_messages_ids_2), 4);

	cIdMapDb = rb_define_class_under(GettextpoHelper, "IdMapDb", rb_cObject);
	rb_define_singleton_method(cIdMapDb, "new", RUBY_METHOD_FUNC(cIdMapDb_new), 1);
	rb_define_method(cIdMapDb, "create", RUBY_METHOD_FUNC(cIdMapDb_create), 1);
	rb_define_method(cIdMapDb, "normalize_database", RUBY_METHOD_FUNC(cIdMapDb_normalize_database), 0);
	rb_define_method(cIdMapDb, "get_min_id", RUBY_METHOD_FUNC(cIdMapDb_get_min_id), 1);
	rb_define_method(cIdMapDb, "get_min_id_array", RUBY_METHOD_FUNC(cIdMapDb_get_min_id_array), 2);
}

}

