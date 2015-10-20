#!/usr/bin/ruby

$:.unshift(File.join(File.dirname(__FILE__)))

require 'tempfile'

require 'sif-lib.rb'
require 'check-lib.rb'
require './gettextpo-helper/ruby-helpers/ruby-helpers.rb'

def custom_basename(input)
  s = (input + '/').match(/(([a-zA-Z0-9\-\_\.%{}]+\/){3})$/)
  if s.nil? or s[1].nil?
    raise "path is too short to determine the basename: #{input}"
  end

  s[1][0..-2]
end

# List of SHA-1 hashes of new contents in a Git commit (for added or modified files)
def contents_of_commit(repo_path, commit_sha1)
  parse_commit_changes(repo_path, commit_sha1).
    select {|x| x[4] != 'D' and x[5].match(/\.pot$/) }.
    map {|x| x[3] }
end

def add_templates_from_repo(src_dir, ids_dir)
  #commits_to_process = git_commits(src_dir) - processed_git_commits(ids_dir)
  inc_proc = IncrementalCommitProcessing.new(src_dir, ids_dir)
  commits_to_process = inc_proc.commits_to_process

  known_broken_pots = []
  begin
    File.open("#{ids_dir}/broken.txt", "r") do |f|
      f.readlines.each do |line|
        if line[0..0] != '#' and line.match(/^[0-9a-f]{40}/)
          known_broken_pots << line[0...40]
        end
      end
    end
  rescue Errno::ENOENT
    # File was not found, but it's OK
  end

  sif = Sif.new(ids_dir)
  tempfile_pot = Tempfile.new(['', '.pot']).path
  commits_to_process.each_with_index do |commit_sha1, index|
    puts ">>> Processing commit #{commit_sha1} (#{index + 1}/#{commits_to_process.size}) in #{src_dir}"
    contents = contents_of_commit(src_dir, commit_sha1)
    broken_in_commit = []

    contents.each do |content_sha1|
      # content_sha1 is the sha1 of blob (e.g. 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4)

      puts "current file SHA1: #{content_sha1}"
      `cd #{src_dir} ; git show #{content_sha1} > "#{tempfile_pot}"`

      pot_status = is_virgin_pot(tempfile_pot)
      if pot_status == :ok
        sif.add(tempfile_pot)
      elsif known_broken_pots.include?(content_sha1)
        puts "Repeated broken POT: #{content_sha1}"
      else
        # Newly appeared broken POT
        puts "Another broken POT: #{content_sha1}"

        known_broken_pots << content_sha1
        broken_in_commit << "#{content_sha1}, :status => :#{pot_status}"
      end
    end

    if broken_in_commit.size > 0
      File.open("#{ids_dir}/broken.txt", "a") do |f|
        f.puts "# From commit #{commit_sha1} in #{src_dir}"
        broken_in_commit.each do |item|
          f.puts item
        end
      end
    end

    inc_proc.add_to_processed_list(commit_sha1)
  end

  `rm -f "#{tempfile_pot}"`
end


if ARGV.size != 2
  puts "Usage: add-templates-from-repo.rb <path-to-git-repo-with-templates> <ids-dir>"
  puts "Example: ./add-templates-from-repo.rb ~/kde-ru/xx-numbering/templates ./ids"
  exit
end

src_dir = ARGV[0] # path to Git repository with translation templates
ids_dir = ARGV[1] # should be already initialized (i.e. run ./sif-init.sh first)

add_templates_from_repo(src_dir, ids_dir)
