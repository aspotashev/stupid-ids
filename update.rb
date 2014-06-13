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
      `git svn init --trunk=/ "#{svn_path}"`
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

    # Rebase "master" on top of "svn/trunk"
    `git rebase trunk`
    raise "git rebase failed" if $?.exitstatus != 0

    # Check than branches "master" and "trunk" are same
    master_sha1 = `git rev-parse master`.strip
    raise "git rev-parse failed" if $?.exitstatus != 0
    trunk_sha1 = `git rev-parse trunk`.strip
    raise "git rev-parse failed" if $?.exitstatus != 0

    if master_sha1 != trunk_sha1
      raise "\"master\" is not pointing to \"trunk\""
    end
  end
end

# Make sure this script is executed from the directory where it is located
if File.expand_path(File.dirname(__FILE__)) != File.expand_path(Dir.pwd)
  raise "Please go to the directory where this script is stored and run it from there"
end

templates_location = "#{Dir.pwd}/xx-numbering"
templates = "#{templates_location}/templates"
templates_stable = "#{templates_location}/stable-templates"
ids = "#{templates_location}/ids"

maintain_git_svn_mirror("svn://anonsvn.kde.org/home/kde/trunk/l10n-kde4/templates", templates)
maintain_git_svn_mirror("svn://anonsvn.kde.org/home/kde/branches/stable/l10n-kde4/templates", templates_stable)

#`bash -c "cd stupid-id-filler && ./add-templates-from-repo.rb #{templates} ./ids"`
if not File.exists?("#{ids}/next_id.txt")
  `./stupid-id-filler/sif-init.sh "#{ids}"`
end

system("./stupid-id-filler/add-templates-from-repo.rb #{templates} #{ids}")
system("./stupid-id-filler/add-templates-from-repo.rb #{templates_stable} #{ids}")

__END__

bash -c "cd stupid-id-filler && ./add-templates-from-repo.rb $TEMPLATES_STABLE ./ids"

bash -c "cd stupid-id-filler/database && ./update-database.rb"

bash -c "cd transition-detector && ./update-database.rb"

