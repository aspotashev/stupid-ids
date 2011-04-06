
Array.class_eval do
	def neighbor_pairs
		(0..(size-2)).map {|i| [self[i], self[i+1]] }
	end
end

def is_virgin_pot_content(content)
	lines = content.split("\n")

	if lines.any? {|line| line.match(/^#\. \+> /) }
		return false # files from PO Summit ( see http://techbase.kde.org/Localization/Workflows/PO_Summit )
	end

	if lines[lines.find_index("msgid \"\"") - 1] != "#, fuzzy"
		return false # header of .POT should be fuzzy
	end

	if (lines + ['']).neighbor_pairs.
		select {|pair| pair[0].match(/^msgstr(\[[0-9]+\])? /) }[1..-1].
		any? {|pair| pair[0].sub(/^msgstr(\[[0-9]+\])? /, '') != "\"\"" or (pair[1] != "" and not pair[1].match(/^msgstr/)) }

		return false # .POT contains translations in 'msgstr' fields
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

