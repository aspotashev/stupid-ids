
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
  output = `cd #{git_dir} ; git show --raw --no-abbrev #{git_ref}`
  raise "git show failed, git_dir = #{git_dir}" if $?.exitstatus != 0

  output.split("\n").
    select {|line| line[0..0] == ':' }.
    map do |line|
      line =~ /^:([0-9]{6}) ([0-9]{6}) ([0-9a-f]{40}) ([0-9a-f]{40}) ([AMD])\t(.+)$/ && $~.captures
    end
end

class TextFileStorage
  def initialize(path)
    @path = path
  end

  def list
    begin
      File.open(@path).read.split("\n")
    rescue Errno::ENOENT => e
      []
    end
  end

  def add(string)
    dir = File.dirname(@path)
    if not File.exists?(dir)
      `mkdir -p "#{dir}"`
    end

    File.open(@path, 'a+') do |f|
      f.puts string
    end
  end
end

class IncrementalCommitProcessing
  # git_dir -- repo from where the commits are taken
  # proc_git_dir -- directory where 'processed.txt' resides

  # The db object is responsible for the persistent storage (file or database)
  # where the list of processed commits is written.
  def initialize(git_dir, db)
    @git_dir = git_dir
    @db = db
  end

  def commits_to_process
    add_git_commits = `cd "#{@git_dir}" ; git log --format=format:%H`.split("\n").reverse
    add_git_commits - @db.list
  end

  def add_to_processed_list(commit_sha1)
    @db.add(commit_sha1)
  end
end
