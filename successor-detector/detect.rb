#!/usr/bin/ruby19
# Ruby 1.9 required for "Dir.exists?(path)"

$SRC_DIR = '/home/sasha/kde-ru/xx-numbering/templates'
$ID_MERGER_REPO = './idmerges'

class PotIdMergePair < Struct.new(:git_dir, :tp_hash_a, :tp_hash_b)
	def self.from_sha1s(git_dir, sha1_a, sha1_b)
		PotIdMergePair.new(git_dir,
			sha1_to_tp_hash(git_dir, sha1_a),
			sha1_to_tp_hash(git_dir, sha1_b))
	end

	def to_s
		"#{tp_hash_a} #{tp_hash_b}"
	end

private
	def self.sha1_to_tp_hash(git_dir, sha1)
		`cd "#{git_dir}" ; git show #{sha1} | ~/stupid-ids/gettextpo-helper/dump-template/dump-template`
	end
end

class PotIdMerge
	def initialize(options)
		raise if not options[:subject] or not options[:author] or not options[:date] or not options[:pairs]

		@options = options
	end

	def to_s
		res = ''
		res << " Subject: " + @options[:subject] + "\n"
		res << " Author: " + @options[:author] + "\n"
		res << " Date: " + @options[:date] + "\n"
		# TODO: check for duplicate .idmerges already existing in the repository (check by Git hash?)
		res << @options[:pairs].map(&:to_s).join("\n") + "\n"
		res
	end

	def empty? # TODO: use metaprogramming for delegation
		@options[:pairs].empty?
	end
end

def detect_template_changes(git_dir, git_ref)
	output = `cd "#{git_dir}" ; git show #{git_ref} --raw --no-abbrev`
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

	lines.map {|m| PotIdMergePair.from_sha1s(git_dir, m[1], m[2]) }
end

def generate_idmerge(git_dir, git_ref)
	sha1 = `cd #{git_dir} ; git log --format=format:%H -1 #{git_ref}`.strip

	PotIdMerge.new(
		:subject => "Translation template(s) changed in commit #{sha1} (detect_template_changes)",
		:author  => "successor detector",
		:date    => Time.now.to_s,
		:pairs   => detect_template_changes(git_dir, sha1))
end

def add_to_merger_repo(id_merger_repo, idmerge)
	if not idmerge.empty?
		if not Dir.exists?(id_merger_repo + '/.git')
			puts "Creating new Git repository for idmerges..."
			puts `mkdir -p "#{id_merger_repo}" && \
				cd "#{id_merger_repo}" && \
				git init && \
				touch .gitignore && \
				git add .gitignore && \
				git commit -m init && \
				git tag init` # tag 'init' is required for software that reads the repository incrementally
		end

		`cd #{id_merger_repo} ; mkdir -p successor-detector`
		filename = "successor-detector/#{Time.now.to_f}.idmerge"
		File.open(id_merger_repo + "/" + filename, 'w') do |f|
			f.puts idmerge.to_s
		end
		`cd #{id_merger_repo} ; git add #{filename} ; git commit -m '[auto] add #{filename}'`
	end
end

def git_commits(dir)
	`cd "#{dir}" ; git log --format=format:%H`.split("\n")
end

def processed_git_commits
	begin
		File.open($ID_MERGER_REPO + '/processed.txt').read.split("\n")
	rescue
		[]
	end
end

puts "Collecting unprocessed commits..."
commits = git_commits($SRC_DIR) - processed_git_commits
puts "#{commits.size} commits left to process"
commits.each do |sha1|
	puts "Processing #{sha1}..."
	add_to_merger_repo($ID_MERGER_REPO, generate_idmerge($SRC_DIR, sha1))

	File.open($ID_MERGER_REPO + '/processed.txt', 'a+') do |f|
		f.puts sha1
	end
end

