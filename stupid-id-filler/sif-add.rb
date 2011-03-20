#!/usr/bin/ruby
#
# Adds to a Git repository:
#  1. the given .pot file
#  2. the first ID for that .pot file
#  3. map: [sha1 of .pot] <-> [name and date of .pot]

require './sif-lib.rb'


dir = ARGV[0]
pot = ARGV[1]
pot_name = ARGV[2]

if dir.nil? or pot.nil? or pot_name.nil?
	puts "Usage: ./sif-add.rb <dir> <.pot> <.pot name>"
	exit
end

sif = Sif.new('ids')
sif.add(pot, pot_name)
sif.commit

