#!/usr/bin/ruby

$:.unshift(File.join(File.dirname(__FILE__)) + '/..')

require 'sif-lib.rb'


if ARGV.size != 1
  puts "Usage: add-templates-from-repo.rb <path-to-git-repo-with-templates>"
  puts "Example: ./add-templates-from-repo.rb ~/kde-ru/xx-numbering/templates"
  exit
end

src_dir = ARGV[0] # path to Git repository with translation templates

sif = Sif.new
sif.add_templates_from_repo(src_dir)
