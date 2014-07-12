
// class Message

#include <gtpo/message.h>

#include "module.h"

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
    return rb_str_new2(rb_get_message(self)->msgstr(FIX2INT(plural_form)).c_str());
}

VALUE cMessage_msgcomments(VALUE self)
{
    return rb_str_new2(rb_get_message(self)->msgcomments().c_str());
}

// init
void init_Message()
{
	cMessage = rb_define_class_under(GettextpoHelper, "Message", rb_cObject);
	rb_define_method(cMessage, "num_plurals", RUBY_METHOD_FUNC(cMessage_num_plurals), 0);
	rb_define_method(cMessage, "equal_translations", RUBY_METHOD_FUNC(cMessage_equal_translations), 1);
	rb_define_method(cMessage, "is_fuzzy", RUBY_METHOD_FUNC(cMessage_is_fuzzy), 0);
	rb_define_method(cMessage, "is_plural", RUBY_METHOD_FUNC(cMessage_is_plural), 0);
	rb_define_method(cMessage, "is_untranslated", RUBY_METHOD_FUNC(cMessage_is_untranslated), 0);
	rb_define_method(cMessage, "msgstr", RUBY_METHOD_FUNC(cMessage_msgstr), 1);
	rb_define_method(cMessage, "msgcomments", RUBY_METHOD_FUNC(cMessage_msgcomments), 0);
}

