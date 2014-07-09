
#include <assert.h>

#include <gtpo/gitloader.h>

#include "module.h"

GitLoader *rb_get_git_loader(VALUE self);

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

// init
void init_GitLoader()
{
	cGitLoader = rb_define_class_under(GettextpoHelper, "GitLoader", rb_cObject);
	rb_define_singleton_method(cGitLoader, "new", RUBY_METHOD_FUNC(cGitLoader_new), 0);
	rb_define_method(cGitLoader, "add_repository", RUBY_METHOD_FUNC(cGitLoader_add_repository), 1);
}

