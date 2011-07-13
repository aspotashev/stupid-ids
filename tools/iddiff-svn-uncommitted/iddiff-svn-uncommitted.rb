#!/usr/bin/ruby19

PATHS = [
  "/home/sasha/messages",
  "/home/sasha/docmessages",
  "/home/sasha/stable-messages",
  "/home/sasha/stable-docmessages",
]

file_list = `svn status #{PATHS.map {|x| "\"#{x}\"" }.join(' ') }`.
	split("\n").
	select {|x| ['A', 'M'].include?(x[0..0]) }

file_list = file_list.map do |line|
	m = line.match(/^([AM]) +(\/.*)$/)
	[m[1], m[2]]
end

file_list = file_list.select {|x| x[1].match(/\.po$/) }


tempfile = `tempfile`.strip
`rm -f "#{tempfile}"`
pofile_a = tempfile + "_a.po"
iddiff_next = tempfile + "_next.iddiff"
iddiff_merged = tempfile + "_merged.iddiff"
trcomm_next = tempfile + "_next.iddiff-trcomm"
trcomm_merged = tempfile + "_merged.iddiff-trcomm"

file_list.each do |file|
	mode, filename = file
	pofile_b = filename
	STDERR.puts "Diffing #{filename}..."

	diff_content = nil
	if mode == 'M'
		`svn cat "#{filename}" > "#{pofile_a}"`

		# TODO: do something with translators' comments
		diff_content = `iddiff "#{pofile_a}" "#{pofile_b}"`
	else # mode == 'A'
		# TODO: do something with translators' comments
		diff_content = `iddiff "#{pofile_b}"`
	end

	if not diff_content.empty?
		File.open(iddiff_next, 'w') do |f|
			f.write(diff_content)
		end

		`iddiff-merge #{iddiff_next} #{iddiff_merged}`
	end

	`mv /tmp/11235.iddiff-tr-comments #{trcomm_next}` if File.exists?("/tmp/11235.iddiff-tr-comments")
	`iddiff-merge-trcomm "#{trcomm_next}" "#{trcomm_merged}"`

	`rm -f #{trcomm_next} #{iddiff_next} #{pofile_a} #{tempfile}`
end

puts iddiff_merged if File.exists?(iddiff_merged)
puts trcomm_merged if File.exists?(trcomm_merged)