// class TranslationContent

#include <assert.h>
#include <gettextpo-helper/translationcontent.h>
#include <gettextpo-helper/gitloader.h>

#include "module.h"
#include "gitloader.h"

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
	assert(git_oid_fromstr(&oid, StringValuePtr(oid_str)) == GIT_SUCCESS);

	TranslationContent *content = new TranslationContent(rb_get_git_loader(git_loader), &oid);
	content->setDisplayFilename("/[git]");
	return Data_Wrap_Struct(cTranslationContent, NULL, free_cTranslationContent, content);
}

TranslationContent *rb_get_translation_content(VALUE self)
{
	TranslationContent *content;
	Data_Get_Struct(self, TranslationContent, content);
	assert(content);

	return content;
}

// init
void init_TranslationContent()
{
	cTranslationContent = rb_define_class_under(GettextpoHelper, "TranslationContent", rb_cObject);
	rb_define_singleton_method(cTranslationContent, "new", RUBY_METHOD_FUNC(cTranslationContent_new_file), 1);
	rb_define_singleton_method(cTranslationContent, "new", RUBY_METHOD_FUNC(cTranslationContent_new_git), 2);
}

