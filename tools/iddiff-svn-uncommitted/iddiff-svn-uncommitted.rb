#!/usr/bin/ruby19

require 'tempfile'
require 'stupidsruby'

PATHS = [
  GettextpoHelper.stupids_conf_path("translation_editing.svn.gui_trunk"),
  GettextpoHelper.stupids_conf_path("translation_editing.svn.doc_trunk"),
  GettextpoHelper.stupids_conf_path("translation_editing.svn.gui_stable"),
  GettextpoHelper.stupids_conf_path("translation_editing.svn.doc_stable"),
]

file_list = `svn status #{PATHS.map {|x| "\"#{x}\"" }.join(' ') }`.
	split("\n").
	select {|x| ['A', 'M'].include?(x[0..0]) }

file_list = file_list.map do |line|
	m = line.match(/^([AM]) +(\/.*)$/)
	[m[1], m[2]]
end

file_list = file_list.select {|x| x[1].match(/\.po$/) }


pofile_a      = Tempfile.new(['', '_a.po']).path
iddiff_next   = Tempfile.new(['', '_next.iddiff']).path
iddiff_merged = Tempfile.new(['', '_merged.iddiff']).path
trcomm_next   = Tempfile.new(['', '_next.iddiff-trcomm']).path
trcomm_merged = Tempfile.new(['', '_merged.iddiff-trcomm']).path

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
end

iddiff_res = '/tmp/res-iddiff'
trcomm_res = '/tmp/res-iddiff-trcomm'
`mv "#{iddiff_merged}" "#{iddiff_res}"` if File.exists?(iddiff_merged)
`mv "#{trcomm_merged}" "#{trcomm_res}"` if File.exists?(trcomm_merged)

puts iddiff_res if File.exists?(iddiff_res)
puts trcomm_res if File.exists?(trcomm_res)
