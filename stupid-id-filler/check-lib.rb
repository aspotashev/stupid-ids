
Array.class_eval do
	def neighbor_pairs
		(0..(size-2)).map {|i| [self[i], self[i+1]] }
	end
end

# Interesting cases: (TODO: think it out)
#    1. http://websvn.kde.org/?view=revision&revision=1207849 -- almost normal .pot, but the header is not fuzzy
#
# TBD: rewrite this using libgettextpo, avoid manual parsing
def is_virgin_pot_content(content, filename)
	if content.empty?
		return :empty_content
	end

	lines = content.split("\n")

	if lines.any? {|line| line.match(/^#\. \+> /) }
		puts "POT file from PO Summit"
		return :po_summit # files from PO Summit ( see http://techbase.kde.org/Localization/Workflows/PO_Summit )
	end

  header_msgid_index = lines.find_index("msgid \"\"")
  if header_msgid_index.nil?
    puts "POT header is missing"
    return :header_missing
  end

  # Check that libgettextpo is able to parse the template/catalog (TBD: do this without calculating tp_hash)
  # TBD: load POT from a buffer, not a file; remove the "filename" argument from this function
  if GettextpoHelper.calculate_tp_hash(filename).empty?
    puts "Failed to parse POT with libgettextpo"
    return :parsing_failed
  end

	if lines[header_msgid_index - 1] != "#, fuzzy"
		puts "Header of POT is not fuzzy"
		return :non_fuzzy_header # header of .POT should be fuzzy
	end

	if (lines + ['']).neighbor_pairs.
		select {|pair| pair[0].match(/^msgstr(\[[0-9]+\])? /) }[1..-1].
		any? {|pair| pair[0].sub(/^msgstr(\[[0-9]+\])? /, '') != "\"\"" or pair[1].match(/^\"/) }

		puts "POT contains translated messages"
		return :has_translations # .POT contains translations in 'msgstr' fields
	end

  if not content.match(/"POT-Creation-Date:\ /)
    puts "POT-Creation-Date is missing in the POT header"
    return :creation_date_missing
  end

	:ok # All tests passed
end

def is_virgin_pot(filename)
	is_virgin_pot_content(IO.read(filename), filename)
end

