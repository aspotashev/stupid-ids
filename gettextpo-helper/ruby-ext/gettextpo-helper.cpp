/* http://www.rubyinside.com/how-to-create-a-ruby-extension-in-c-in-under-5-minutes-100.html */

#include "ruby.h"

#include "../include/gettextpo-helper.h"

void init_search(const char *f_dump, const char *f_index, const char *f_map);
const char *find_string_id_by_str_multiple(char *s, int n);

//-------------------------------

/*VALUE wrap_init_search(VALUE self, VALUE dump, VALUE index, VALUE map)
{
	init_search(StringValuePtr(dump), StringValuePtr(index), StringValuePtr(map));
	return Qnil;
}

VALUE wrap_find_string_id(VALUE self, VALUE str, VALUE num)
{
	int n = FIX2INT(num);

	VALUE res = rb_ary_new(); // create array
	for (int index = get_internal_index_by_string(StringValuePtr(str)) - 3, i = 0; // HACK
		i < n; index ++, i ++)
	{
		// append to array
		rb_ary_push(res, rb_str_new2(get_msg_id_by_internal_index(index)));
	}
	return res;
}*/

VALUE wrap_calculate_tp_hash(VALUE self, VALUE filename)
{
	std::string res = calculate_tp_hash(StringValuePtr(filename));
	return rb_str_new2(res.c_str());
}

VALUE wrap_get_pot_length(VALUE self, VALUE filename)
{
	return INT2FIX(get_pot_length(StringValuePtr(filename)));
}

extern "C" {

/* Function called at module loading */
void Init_gettextpo_helper()
{
	VALUE GettextpoHelper = rb_define_module("GettextpoHelper");
	rb_define_singleton_method(GettextpoHelper, "calculate_tp_hash", RUBY_METHOD_FUNC(wrap_calculate_tp_hash), 1);
	rb_define_singleton_method(GettextpoHelper, "get_pot_length", RUBY_METHOD_FUNC(wrap_get_pot_length), 1);
//	rb_define_singleton_method(GettextpoHelper, "find", RUBY_METHOD_FUNC(wrap_find_string_id), 2);
}

}

