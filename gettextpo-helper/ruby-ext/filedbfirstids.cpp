
#include <assert.h>
#include <gtpo/filedbfirstids.h>

#include "module.h"

void free_cFiledbFirstIds(void *obj)
{
	delete (FiledbFirstIds *)obj;
}

VALUE cFiledbFirstIds;

VALUE cFiledbFirstIds_new(VALUE klass, VALUE filename, VALUE filename_next_id)
{
	FiledbFirstIds *obj = new FiledbFirstIds(StringValuePtr(filename), StringValuePtr(filename_next_id));
	return Data_Wrap_Struct(cFiledbFirstIds, NULL, free_cFiledbFirstIds, obj);
}

FiledbFirstIds *rb_get_file_db_first_ids(VALUE self)
{
	FiledbFirstIds *obj;
	Data_Get_Struct(self, FiledbFirstIds, obj);
	assert(obj);

	return obj;
}

VALUE cFiledbFirstIds_get_first_id(VALUE self, VALUE tp_hash)
{
	std::pair<int, int> res_pair = rb_get_file_db_first_ids(self)->getFirstId(GitOid(StringValuePtr(tp_hash)));

	VALUE res = rb_ary_new();
	rb_ary_push(res, INT2FIX(res_pair.first));
	rb_ary_push(res, INT2FIX(res_pair.second));
	return res;
}

// init
void init_FiledbFirstIds()
{
	cFiledbFirstIds = rb_define_class_under(GettextpoHelper, "FiledbFirstIds", rb_cObject);
	rb_define_singleton_method(cFiledbFirstIds, "new", RUBY_METHOD_FUNC(cFiledbFirstIds_new), 2);
	rb_define_method(cFiledbFirstIds, "get_first_id", RUBY_METHOD_FUNC(cFiledbFirstIds_get_first_id), 1);
}

