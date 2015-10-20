#!/usr/bin/ruby

$:.unshift(File.join(File.dirname(__FILE__)))

require 'tempfile'

require 'sif-lib.rb'
require 'check-lib.rb'
require './gettextpo-helper/ruby-helpers/ruby-helpers.rb'


if ARGV.size != 2
  puts "Usage: add-templates-from-repo.rb <path-to-git-repo-with-templates> <ids-dir>"
  puts "Example: ./add-templates-from-repo.rb ~/kde-ru/xx-numbering/templates ./ids"
  exit
end

src_dir = ARGV[0] # path to Git repository with translation templates
ids_dir = ARGV[1] # should be already initialized (i.e. run ./sif-init.sh first)

sif = Sif.new(ids_dir)
sif.add_templates_from_repo(src_dir, ids_dir)
