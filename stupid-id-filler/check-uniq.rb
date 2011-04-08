#!/usr/bin/ruby

# Needs a rewrite (by the way, duplicate hashes of POTs are not fatal)
__END__
def list_non_unique_basename_potdate(ids_dir)
        pairs = IO.read(ids_dir + '/pot_names.txt').split("\n").
                map {|x| [x[0..39], x[41..-1]] }.
                sort_by(&:last)

        res = []
        (0...(pairs.size-1)).each do |i|
                if pairs[i][1] == pairs[i+1][1]
                        res << pairs[i..i+1]
                end
        end

        res
end

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

