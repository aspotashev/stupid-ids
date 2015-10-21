#!/usr/bin/ruby
#
# Adds to Couchbase:
#  1. the first ID for template-part hash of this .pot file
#  2. map: [sha1 of .pot] <-> [template-part hash]

require '../sif-lib.rb'

pot = ARGV[0]

if pot.nil?
  puts "Usage: ./sif-add.rb <.pot>"
  exit
end

sif = Sif.new
sif.add(pot)
