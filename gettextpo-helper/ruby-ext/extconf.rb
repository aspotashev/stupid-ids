
# /* http://www.rubyinside.com/how-to-create-a-ruby-extension-in-c-in-under-5-minutes-100.html */
require 'mkmf'
extension_name = 'gettextpo_helper'
dir_config(extension_name)
have_library('cryptopp')
have_library('gettextpo')
create_makefile(extension_name)
