#!/usr/bin/ruby

def maintain_git_svn_mirror(svn_path, git_path)
  puts "Mirroring #{svn_path} to #{git_path}"
  `mkdir -p "#{git_path}"`
  raise "mkdir failed" if $?.exitstatus != 0

  # Go to this directory and do stuff there
  Dir.chdir(git_path) do
    # Make sure there is a Git repo
    # http://stackoverflow.com/questions/957928/is-there-a-way-to-get-the-git-root-directory-in-one-command
    git_toplevel = `git rev-parse --show-toplevel`.strip
    if $?.exitstatus != 0 or git_toplevel != git_path
      puts "Git repo does not exist"

      # Make sure directory is empty
      if not (Dir.entries('.') - %w{. ..}).empty?
        raise "Directory #{git_path} is not empty, exiting to prevent overwriting files in it"
      end

      # Initialize Git-SVN
      `git svn init "#{svn_path}"`
      raise "git svn init failed" if $?.exitstatus != 0
    else
      puts "Git repo exists"
    end

    status_res = `git status --porcelain`
    if $?.exitstatus != 0 or not status_res.empty?
      raise "Git repo is not clean, i.e. there are uncommitted changes"
    end

    # Fetch from SVN
    `git svn fetch`
    raise "git svn fetch failed" if $?.exitstatus != 0

    # Make sure current branch is "master"
    cur_branch = `git rev-parse --abbrev-ref HEAD`.strip
    raise "git failed" if $?.exitstatus != 0

    if cur_branch != "master"
      raise "Someone has changed to a branch other than \"master\""
    end

    # Rebase "master" on top of latest SVN revision
    `git svn rebase`
    raise "git svn rebase failed" if $?.exitstatus != 0
  end
end

# Make sure this script is executed from the directory where it is located
if File.expand_path(File.dirname(__FILE__)) != File.expand_path(Dir.pwd)
  raise "Please go to the directory where this script is stored and run it from there"
end

templates_location = "#{Dir.pwd}/xx-numbering"
ids = "#{templates_location}/ids"

GitSvnMirror = Struct.new(:svn_path, :local_path)

mirrors = [
  GitSvnMirror.new("svn://anonsvn.kde.org/home/kde/trunk/l10n-kde4/templates", "#{templates_location}/templates"),
  GitSvnMirror.new("svn://anonsvn.kde.org/home/kde/branches/stable/l10n-kde4/templates", "#{templates_location}/stable-templates"),
  GitSvnMirror.new("svn://anonsvn.kde.org/home/kde/trunk/l10n-kf5/templates", "#{templates_location}/kf5-templates"),
  GitSvnMirror.new("svn://anonsvn.kde.org/home/kde/branches/stable/l10n-kf5/templates", "#{templates_location}/kf5-stable-templates"),
]

mirrors.each do |m|
  maintain_git_svn_mirror(m.svn_path, m.local_path)
end

if not File.exists?("#{ids}/next_id.txt")
  `./stupid-id-filler/sif-init.sh "#{ids}"`
end

mirrors.each do |m|
  if not system("./stupid-id-filler/add-templates-from-repo.rb #{m.local_path} #{ids}")
    exit
  end
end

system("cd stupid-id-filler/database && ./update-database.rb")

system("cd transition-detector && ./update-database.rb")

