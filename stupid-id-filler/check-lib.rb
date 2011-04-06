
def is_virgin_pot_content(content)
	lines = content.split("\n")

	if lines.any? {|line| line.match(/^#\. \+> /) }
		return false # files from PO Summit ( see http://techbase.kde.org/Localization/Workflows/PO_Summit )
	end

	true # All tests passed
end

def is_virgin_pot(filename)
	is_virgin_pot_content(IO.read(filename))
end

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

