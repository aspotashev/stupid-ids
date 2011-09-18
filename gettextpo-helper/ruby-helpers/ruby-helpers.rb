
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

# List of changes in a Git commit
#
# result[i][0] -- A mode
# result[i][1] -- B mode
# result[i][2] -- A sha1
# result[i][3] -- B sha1
# result[i][4] -- operation
# result[i][5] -- filename
def parse_commit_changes(git_dir, git_ref)
  `cd #{git_dir} ; git show --raw --no-abbrev #{git_ref}`.split("\n").
    select {|line| line[0..0] == ':' }.
    map do |line|
      line =~ /^:([0-9]{6}) ([0-9]{6}) ([0-9a-f]{40}) ([0-9a-f]{40}) ([AMD])\t(.+)$/ && $~.captures
    end
end

class IncrementalCommitProcessing < Struct.new(:git_dir, :proc_git_dir)
  # git_dir -- repo from where the commits are taken
  # proc_git_dir -- directory where 'processed.txt' resides

  def commits_to_process
    _git_commits(git_dir) - _processed_git_commits(proc_git_dir)
  end

  def add_to_processed_list(commit_sha1)
    _add_to_processed_list(proc_git_dir, commit_sha1)
  end

private
  # List of all commits in git_dir, from the oldest to the newest
  def _git_commits(git_dir)
    `cd "#{git_dir}" ; git log --format=format:%H`.split("\n").reverse
  end

  # List of SHA-1s from processed.txt
  def _processed_git_commits(git_dir)
    begin
      File.open(git_dir + '/processed.txt').read.split("\n")
    rescue Errno::ENOENT => e
      []
    end
  end

  # Add SHA-1 to processed.txt
  def _add_to_processed_list(git_dir, commit_sha1)
    if not File.exists?(git_dir)
      `mkdir -p "#{git_dir}"`
    end

    File.open(git_dir + '/processed.txt', 'a+') do |f|
      f.puts commit_sha1
    end
  end
end

