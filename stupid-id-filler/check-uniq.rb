#!/usr/bin/ruby

require './check-lib.rb'

# Checks if all pairs {basename, POT date} are unique
# (if this fails, stupid-ids should not be used!)

ids_dir = "ids"

duplicates = list_non_unique_basename_potdate(ids_dir)
duplicates.each do |x|
	a, b = *x

	puts "EPIC FAIL: A: #{x[0][0]} #{x[0][1]}"
	puts "EPIC FAIL: B: #{x[1][0]} #{x[1][1]}"
	puts
end

if duplicates.empty?
	puts "Everything is OK"
end

