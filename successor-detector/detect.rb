#!/usr/bin/ruby19
# Ruby 1.9 required for "Dir.exists?(path)"

require 'stupidsruby'
require '../gettextpo-helper/ruby-helpers/ruby-helpers.rb'
require '../stupid-id-filler/check-lib.rb'

raise if ARGV.size != 1 # the SRC_DIR should be given

$SRC_DIR = ARGV[0]
$ID_MERGER_REPO = './idmerges'

raise if not Dir.exists?($SRC_DIR + '/.git')

class PotIdMergePair < Struct.new(:git_dir, :tp_hash_a, :tp_hash_b)
	def self.from_sha1s(git_dir, sha1_a, sha1_b)
		PotIdMergePair.new(git_dir,
			sha1_to_tp_hash(git_dir, sha1_a),
			sha1_to_tp_hash(git_dir, sha1_b))
	end

	def to_s
		if tp_hash_a.nil? or tp_hash_b.nil?
			nil
		elsif tp_hash_a == tp_hash_b # not nil and equal
			raise
		else
			"#{tp_hash_a} #{tp_hash_b}"
		end
	end

private
	def self.sha1_to_tp_hash(git_dir, sha1)
		tempfile_pot = `tempfile --suffix=.pot`.strip
		`cd "#{git_dir}" ; git show #{sha1} > "#{tempfile_pot}"`

		res = nil
		if is_virgin_pot(tempfile_pot) == :ok
			# TODO: GettextpoHelper.calculate_tp_hash should be able to read .pot from buffer (not only from a file)
			res = GettextpoHelper.calculate_tp_hash(tempfile_pot)
		end

		`rm -f "#{tempfile_pot}"`
		res
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
		res << @options[:pairs].map(&:to_s).compact.join("\n") + "\n"
		res
	end

	def empty? # TODO: use metaprogramming for delegation
		@options[:pairs].empty?
	end
end

def detect_template_changes(git_dir, git_ref)
	lines = parse_commit_changes(git_dir, git_ref).
		select {|x| x[4] == 'M' }
	lines.each do |x|
		if x[5].match(/\.pot$/)
			# OK, we have a translation template
		elsif x[5].match(/\.po$/)
			# translation file -- this is a mistake, it will not be processed
			puts "WARNING: .po file in templates/"
		else
			raise "file extension is not .pot"
		end
	end

	lines.map {|m| PotIdMergePair.from_sha1s(git_dir, m[2], m[3]) }
end

def generate_idmerge(git_dir, git_ref)
	sha1 = git_ref_sha1(git_dir, git_ref)

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

puts "Collecting unprocessed commits..."
#commits = git_commits($SRC_DIR) - processed_git_commits($ID_MERGER_REPO)
inc_proc = IncrementalCommitProcessing.new($SRC_DIR, $ID_MERGER_REPO)
commits = inc_proc.commits_to_process
commits.each_with_index do |sha1, index|
	puts "Processing #{sha1} (#{index + 1}/#{commits.size})"
	add_to_merger_repo($ID_MERGER_REPO, generate_idmerge($SRC_DIR, sha1))

	inc_proc.add_to_processed_list(sha1)
end

