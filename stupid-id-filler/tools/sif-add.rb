#!/usr/bin/ruby
#
# Adds to Redis:
#  1. the first ID for template-part hash of this .pot file
#  2. map: [sha1 of .pot] <-> [template-part hash]

$:.unshift(File.join(File.dirname(__FILE__)) + '/..')

require 'sif-lib.rb'

pot = ARGV[0]

if pot.nil?
  puts "Usage: ./sif-add.rb <.pot>"
  exit
end

sif = Sif.new
sif.add(pot)
