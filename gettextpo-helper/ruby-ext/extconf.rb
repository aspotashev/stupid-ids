
# /* http://www.rubyinside.com/how-to-create-a-ruby-extension-in-c-in-under-5-minutes-100.html */
require 'mkmf'

#def try_func(func, libs, headers = nil, &b) # FIXME (hack!)
#	true
#end

extension_name = 'gettextpo_helper'
dir_config(extension_name)
have_library('gettextpo_helper')
create_makefile(extension_name)
