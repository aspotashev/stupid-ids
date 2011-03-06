#!/usr/bin/ruby18

$GIT_DIR = '../templates/'
$ID_MERGER_REPO = '../stupid-id-merger/id-merger-repo'

class PotIdMerge < Struct.new(:git_dir, :name_a, :date_a, :name_b, :date_b)
	def self.from_sha1s_and_name(git_dir, sha1_a, sha1_b, basename)
		PotIdMerge.new(git_dir,
			basename, sha1_to_date(git_dir, sha1_a),
			basename, sha1_to_date(git_dir, sha1_b))
	end

	def to_s
		"#{name_a} <#{date_a}> -- #{name_b} <#{date_b}>"
	end

private
	def self.sha1_to_date(git_dir, sha1)
#		tempfile_pot = `tempfile -s .pot`.strip
#		`cd "#{git_dir}" ; git show #{sha1} > #{tempfile_pot} ; ../stupid-id-filler/get_pot_date.py #{tempfile_pot} ; rm #{tempfile_pot}`.rstrip
		`cd "#{git_dir}" ; git show #{sha1} | grep '^"POT-Creation-Date: '`.match(/^"POT-Creation-Date: (.*)\\n"\n$/)[1]
	end
end

def detect_template_changes(git_dir, ref)
	output = `cd "#{git_dir}" ; git show #{ref} --raw --no-abbrev`
	output = output.split("\n").select {|x| x[0..0] == ':' }

	lines = output.
		map {|x| x.match(/^:[0-9]{6} [0-9]{6} ([0-9a-f]{40}) ([0-9a-f]{40}) (.)\t(.+)$/) }.
		select {|m| m[3] == 'M' }
	lines.each do |m|
		if m[4].match(/\.pot$/)
			# OK, we have a translation template
		elsif m[4].match(/\.po$/)
			# translation file -- this is a mistake, it will not be processed
			puts "WARNING: .po file in templates/"
		else
			raise "file extension is not .pot"
		end
	end

	lines.map {|m| PotIdMerge.from_sha1s_and_name(git_dir, m[1], m[2], File.basename(m[4]).sub(/\.pot$/, '')) }
end

def generate_idmerge(git_dir, ref)
	sha1 = `cd #{git_dir} ; git log --format=format:%H -1 #{ref}`.strip

	res = ''
	res << " Subject: " + "Translation template changed in commit #{sha1} (detect_template_changes)" + "\n"
	res << " Author: " + "successor detector" + "\n"
	res << " Date: " + Time.now.to_s + "\n"
	# TODO: check for duplicate .idmerges already existing in the repository
	res << detect_template_changes(git_dir, sha1).map(&:to_s).join("\n") + "\n"
	res
end

def add_to_merger_repo(id_merger_repo, idmerge_content)
	`cd #{id_merger_repo} ; mkdir -p successor-detector`
	filename = "successor-detector/#{Time.now.to_f}.idmerge"
	File.open(id_merger_repo + "/" + filename, 'w') do |f|
		f.puts idmerge_content
	end
	`cd #{id_merger_repo} ; git add #{filename} ; git commit -m 'add #{filename}'`
end

def git_commits(dir)
	`cd "#{dir}" ; git log --format=format:%H`.split("\n")
end

def processed_git_commits
	begin
		File.open('processed.txt').read.split("\n")
	rescue
		[]
	end
end

commits = git_commits($GIT_DIR) - processed_git_commits
commits.each do |sha1|
	puts "Processing #{sha1}..."
	add_to_merger_repo($ID_MERGER_REPO, generate_idmerge($GIT_DIR, sha1))

	File.open('processed.txt', 'a+') do |f|
		f.puts sha1
	end
end

