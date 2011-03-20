#!/usr/bin/ruby

$SRC_DIR = '~/kde-ru/xx-numbering/templates'
$IDS_DIR = './ids'

#def lstree_output_to_pairs(input)
#  input.split("\n").
#    map {|x| x.match(/^[0-9]{6} blob ([0-9a-f]{40})\t(.+)$/) }.
#    select {|x| not x.nil? }.
#    map {|m| [m[1], m[2]] }.
#    uniq.
#    map {|x| [x[0], x[1].match(/(^|\/)([^\/]+)\.pot$/)[2]] }.
#    uniq
#end

def git_list_of_file_contents
  res = `cd #{$SRC_DIR} ; (for n in $(git log --format=format:%T) ; do git ls-tree -r --full-tree $n ; done) | sort | uniq`
  lstree_output_to_pairs(res)
end

#p git_list_of_file_contents

def list_of_processed
	f = begin
		File.open($IDS_DIR + '/processed.txt', 'r')
	rescue Errno::ENOENT => e
		nil
	end

	f ? f.read.split("\n") : []
end

# List of all commits in $SRC_DIR, from the oldest to the newest
def list_of_all_commits
	`cd #{$SRC_DIR} ; git log --format=format:%H`. # commit hash
		split("\n").reverse
end

def add_to_processed_list(commit_sha1)
	File.open($IDS_DIR + '/processed.txt', 'a+') do |f|
		f.puts commit_sha1
	end
end

def parse_commit_changes(commit_sha1)
	`cd #{$SRC_DIR} ; git show --raw --no-abbrev #{commit_sha1}`.split("\n").
		select {|line| line[0..0] == ':' }.
		map do |line|
			line =~ /^:([0-9]{6}) ([0-9]{6}) ([0-9a-f]{40}) ([0-9a-f]{40}) ([AMD])\t(.+)$/ && $~.captures
		end
end

def contents_of_commit(commit_sha1)
	parse_commit_changes(commit_sha1).
		select {|x| x[4] != 'D' }.
		map {|x| [x[3], File.basename(x[5]).sub(/\.pot$/, '')] }
end

commits_to_process = list_of_all_commits - list_of_processed

tempfile_pot = `tempfile --suffix=.pot`.strip
commits_to_process.each do |commit_sha1|
	puts ">>> Processing commit #{commit_sha1}"
	contents = contents_of_commit(commit_sha1)
	contents.each do |content|
		sha1 = content[0]     # e.g., 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4 (sha1 of blob)
		basename = content[1] # e.g., desktop_kdeaccessibility.pot

		p content
		`cd #{$SRC_DIR} ; git show #{content[0]} > "#{tempfile_pot}"`
		puts `./sif-add.rb "#{$IDS_DIR}" "#{tempfile_pot}" "#{basename}"`
#		sleep 1
	end

	add_to_processed_list(commit_sha1)
end

`rm -f "#{tempfile_pot}"`

