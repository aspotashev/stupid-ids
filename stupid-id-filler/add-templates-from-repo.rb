#!/usr/bin/ruby

require './sif-lib.rb'
require './check-lib.rb'

if ARGV.size != 2
	puts "Usage: add-templates-from-repo.rb <path-to-git-repo-with-templates> <ids-dir>"
	puts "Example: ./add-templates-from-repo.rb ~/kde-ru/xx-numbering/templates ./ids"
	exit
end

$SRC_DIR = ARGV[0] # path to Git repository with translation templates
$IDS_DIR = ARGV[1] # should be already initialized (i.e. run ./sif-init.sh first)


# List of SHA-1s from processed.txt
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

# Add SHA-1 to processed.txt
def add_to_processed_list(commit_sha1)
	File.open($IDS_DIR + '/processed.txt', 'a+') do |f|
		f.puts commit_sha1
	end
end

# List of changes in a Git commit
def parse_commit_changes(commit_sha1)
	`cd #{$SRC_DIR} ; git show --raw --no-abbrev #{commit_sha1}`.split("\n").
		select {|line| line[0..0] == ':' }.
		map do |line|
			line =~ /^:([0-9]{6}) ([0-9]{6}) ([0-9a-f]{40}) ([0-9a-f]{40}) ([AMD])\t(.+)$/ && $~.captures
		end
end

def custom_basename(input)
	s = (input + '/').match(/(([a-zA-Z0-9\-\_\.%{}]+\/){3})$/)
	if s.nil? or s[1].nil?
		raise "path is too short to determine the basename: #{input}"
	end

	s[1][0..-2]
end

# List of SHA-1 hashes of new contents in a Git commit (for added or modified files)
def contents_of_commit(commit_sha1)
	parse_commit_changes(commit_sha1).
		select {|x| x[4] != 'D' and x[5].match(/\.pot$/) }.
		map {|x| x[3] }
end

commits_to_process = list_of_all_commits - list_of_processed

sif = Sif.new($IDS_DIR)
tempfile_pot = `tempfile --suffix=.pot`.strip
commits_to_process.each do |commit_sha1|
	puts ">>> Processing commit #{commit_sha1}"
	contents = contents_of_commit(commit_sha1)
	contents.each do |content_sha1|
		# content_sha1 is the sha1 of blob (e.g. 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4)

		p content_sha1
		`cd #{$SRC_DIR} ; git show #{content_sha1} > "#{tempfile_pot}"`
		if is_virgin_pot(tempfile_pot)
			sif.add(tempfile_pot, :pot_hash => content_sha1)
		end
	end

	sif.commit

	add_to_processed_list(commit_sha1)
end

`rm -f "#{tempfile_pot}"`

