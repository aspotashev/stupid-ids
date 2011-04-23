
require 'open3'

# Returns the Git hash of a .pot file by its tp_hash.
def get_pot_git_hash(tp_hash)
	# perform checks
	all_rows = TphashPotsha.find(:all, :conditions => {:tp_hash => tp_hash})
	if all_rows.size > 1
		p all_rows
		raise "There should not be more than one .pot file with the same tp_hash"
	elsif all_rows.size == 0
		raise "tp_hash not found: tp_hash = #{tp_hash}"
	end

	# simply do the job
	TphashPotsha.find(:first, :conditions => {:tp_hash => tp_hash}).potsha
end

def try_extract_pot_to_file(git_dir, git_hash, filename)
	Open3.popen3("cd \"#{git_dir}\" ; git show #{git_hash} --") do |stdin, stdout, stderr|
		stdout_data = stdout.read # reading in this order to avoid blocking
		stderr_data = stderr.read

		if not stderr_data.empty? # git object not found
			return false
		else
			File.open(filename, 'w') do |f|
				f.write(stdout_data)
			end

			return true # OK
		end
	end
end

# filename -- where to write the .pot
def extract_pot_to_file(tp_hash, filename)
	git_hash = get_pot_git_hash(tp_hash)

	['~/kde-ru/xx-numbering/templates', '~/kde-ru/xx-numbering/stable-templates'].
		map {|path| File.expand_path(path) }.
		any? do |git_dir|

		try_extract_pot_to_file(git_dir, git_hash, filename)
	end
end

#-------------- Git -----------------

def git_head_sha1(git_dir)
	git_ref_sha1(git_dir, 'HEAD')
end

def git_ref_sha1(git_dir, ref)
	`cd "#{git_dir}" ; git log --format=format:%H -1 #{ref}`.strip
end

# ref1 must be in the "git log #{ref2}" (i.e. ref1 is earlier than ref2)
def git_commits_between(git_dir, ref1, ref2)
	ref1 = git_ref_sha1(git_dir, ref1)
	ref2 = git_ref_sha1(git_dir, ref2)

	log = `cd "#{git_dir}" ; git log --format=format:%H #{ref2} --`.split("\n")
	index = log.index(ref1)

	raise if index.nil?

	log[0...index].reverse
end

