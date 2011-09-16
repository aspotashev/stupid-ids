// class IdMapDb

#include <assert.h>
#include <gettextpo-helper/mappedfile.h>

#include "module.h"

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

// init
void init_IdMapDb()
{
	cIdMapDb = rb_define_class_under(GettextpoHelper, "IdMapDb", rb_cObject);
	rb_define_singleton_method(cIdMapDb, "new", RUBY_METHOD_FUNC(cIdMapDb_new), 1);
	rb_define_method(cIdMapDb, "create", RUBY_METHOD_FUNC(cIdMapDb_create), 1);
	rb_define_method(cIdMapDb, "normalize_database", RUBY_METHOD_FUNC(cIdMapDb_normalize_database), 0);
	rb_define_method(cIdMapDb, "get_min_id", RUBY_METHOD_FUNC(cIdMapDb_get_min_id), 1);
	rb_define_method(cIdMapDb, "get_min_id_array", RUBY_METHOD_FUNC(cIdMapDb_get_min_id_array), 2);
}

