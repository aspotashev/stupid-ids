#!/usr/bin/ruby

require './sif-lib.rb'
require './check-lib.rb'
require '../gettextpo-helper/ruby-helpers/ruby-helpers.rb'

if ARGV.size != 2
	puts "Usage: add-templates-from-repo.rb <path-to-git-repo-with-templates> <ids-dir>"
	puts "Example: ./add-templates-from-repo.rb ~/kde-ru/xx-numbering/templates ./ids"
	exit
end

$SRC_DIR = ARGV[0] # path to Git repository with translation templates
$IDS_DIR = ARGV[1] # should be already initialized (i.e. run ./sif-init.sh first)


def custom_basename(input)
	s = (input + '/').match(/(([a-zA-Z0-9\-\_\.%{}]+\/){3})$/)
	if s.nil? or s[1].nil?
		raise "path is too short to determine the basename: #{input}"
	end

	s[1][0..-2]
end

# List of SHA-1 hashes of new contents in a Git commit (for added or modified files)
def contents_of_commit(commit_sha1)
	parse_commit_changes($SRC_DIR, commit_sha1).
		select {|x| x[4] != 'D' and x[5].match(/\.pot$/) }.
		map {|x| x[3] }
end

commits_to_process = git_commits($SRC_DIR) - processed_git_commits($IDS_DIR)

sif = Sif.new($IDS_DIR)
tempfile_pot = `tempfile --suffix=.pot`.strip
commits_to_process.each_with_index do |commit_sha1, index|
	puts ">>> Processing commit #{commit_sha1} (#{index + 1}/#{commits_to_process.size})"
	contents = contents_of_commit(commit_sha1)
	contents.each do |content_sha1|
		# content_sha1 is the sha1 of blob (e.g. 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4)

		p content_sha1
		`cd #{$SRC_DIR} ; git show #{content_sha1} > "#{tempfile_pot}"`
		if is_virgin_pot(tempfile_pot) == :ok
			sif.add(tempfile_pot, :pot_hash => content_sha1)
		else
			white_list = [ # known really broken POTs
				'c858f9fb3d0879344e020e8da6e75b49ec69f422', # POTs with translations
				'367d7b7c18ba66a6d308a92401f79e96a8b50e0c',
				'ed6cfadfaa021938da118044555e7f0112bb32cd',
				'72358faf60b872e2113041a5ecaa1b0c8ae9bf73',
				'4c1c3e3ff0c853256401dbc910cbc70050904cce',
				'941f68ea0de6e37d3bbec96720ee4a5929b589a1',
				'dfc6efaec41640efd65f08105b32d9cd3e5c6493',
				'7102486a99154cf41d21ee8c29475bfc263c17fc',
				'92fa2380247f1445123c79d1b0562bdb40758e26',
				'7ead16f41b58e4e4b63473c8b84664d90ac79255',
				'66767c4cfc5e349d78da99c9fa4fb88d9edd0a79',
				'39794651da0335a6017f6b8d94a3e292fce106a5',
				'f59b8f79171b52986d8b44e98117b398fafa8384',

				'24c554289355b4b805ab1654fbe5c52ebe02f905', # header is not clean
				'62e7afe093409b51fb688484e1f44ad3d6dfc336',

				'b5fbe988b1355356528c10fd3c55041928a6afa6', # broken header
			]

			if is_virgin_pot(tempfile_pot) != :po_summit and not white_list.include?(content_sha1)
				raise "Broken .pot (content sha1: #{content_sha1})"
			end
		end
	end

	sif.commit

	add_to_processed_list($IDS_DIR, commit_sha1)

	if index % 30 == 0
		`cd #{$IDS_DIR} ; git gc`
	end
end

`rm -f "#{tempfile_pot}"`

