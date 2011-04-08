#!/usr/bin/ruby

# TODO:
#  1. rewrite to remove broken POTs (POTs from PO Summit, POTs with translations).
#  2. rename this script to "remove-broken-templates.rb"

__END__
require 'set'
require './check-lib.rb'

$SRC_DIR = '/home/sasha/kde-ru/xx-numbering/templates'
$IDS_DIR = 'ids'

def is_virgin_pot_by_sha1(sha1)
	is_virgin_pot_content(`cd #{$SRC_DIR} ; git show #{sha1}`)
end

#def remove_all_by_sha1s(ids_dir, sha1s)
#end

sha1s = list_non_unique_basename_potdate($IDS_DIR).
	map {|pair| [pair[0][0], pair[1][0]] }.
	flatten.uniq

p sha1s.size

#sha1s = sha1s[0..300] # FIXME

i = 0
sha1s = sha1s.select do |sha1|
	if i % 20 == 0
		print "\b"*100
		print "Counting non-POT contents (#{i.to_s.rjust(sha1s.size.to_s.size)}/#{sha1s.size})"
		$stdout.flush
	end
	i += 1

	not is_virgin_pot_by_sha1(sha1)
end
puts

sha1s = sha1s.to_set

pot_names = IO.read($IDS_DIR + '/pot_names.txt').split("\n").
	select {|x| not sha1s.include?(x[0..39]) }
File.open($IDS_DIR + '/pot_names.txt', 'w') do |f|
	f.puts pot_names
end

first_ids = IO.read($IDS_DIR + '/first_ids.txt').split("\n").
	select {|x| not sha1s.include?(x[0..39]) }
File.open($IDS_DIR + '/first_ids.txt', 'w') do |f|
	f.puts first_ids
end

puts `cd "#{$IDS_DIR}" ; git commit -m "removing broken POTs (POTs from PO Summit, POTs with translations [TODO])" -- first_ids.txt pot_names.txt`

