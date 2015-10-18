#!/usr/bin/ruby
#
# Adds to a Git repository:
#  1. the first ID for template-part hash of this .pot file
#  2. map: [sha1 of .pot] <-> [template-part hash]

require '../sif-lib.rb'


dir = ARGV[0]
pot = ARGV[1]

if dir.nil? or pot.nil?
	puts "Usage: ./sif-add.rb <dir> <.pot>"
	exit
end

sif = Sif.new(dir)
sif.add(pot)
sif.commit

