
$:.unshift(File.join(File.dirname(__FILE__)) + "/../build/gettextpo-helper/ruby-ext")
require 'stupidsruby'

$:.unshift(File.join(File.dirname(__FILE__)) + "/../gettextpo-helper/ruby-helpers")
require 'ruby-helpers.rb'

require 'tempfile'
require 'check-lib.rb'
require 'set'
require 'securerandom'
require 'redis'

class Sif
  # http://www.oreillynet.com/ruby/blog/2007/01/nubygems_dont_use_class_variab_1.html
  @object_created = false
  class << self; attr_accessor :object_created; end

  def initialize
    if self.class.object_created
      raise "can create only one instance of class Sif (to prevent conflicting changes in files)"
    else
      self.class.object_created = true
    end

    @redis = Redis.new

    @next_id = @redis.get('next_id').to_i
    if @next_id.nil?
      @next_id = @redis.hgetall('tphash_to_idrange').map {|k,v| v.split.map(&:to_i).inject(&:+) }.max || 100
      @redis.set('next_id', @next_id)
    end

    @known_broken_pots = load_known_broken_pots
  end

  def self.git_hash_object(path)
    data = "blob #{File.size(path).to_s(10)}\0" + File.open(path).read
    Digest::SHA1.hexdigest(data)
  end

  def have_info_for_pot_hash?(pot_hash)
    tphash = @redis.hget('pot_to_tohash', pot_hash)
    return false if tphash.nil?

    not @redis.hget('tphash_to_idrange', tphash).nil?
  end

  def add(pot_path)
    pot_hash = Sif.git_hash_object(pot_path)

    if not @redis.hexists('pot_to_tohash', pot_hash)
      tphash = GettextpoHelper.calculate_tp_hash(pot_path)
      raise if tphash.size != 40

      @redis.hset('pot_to_tohash', pot_hash, tphash)

      if @redis.hexists('tphash_to_idrange', tphash)
        puts "This template-part hash already exists in Couchbase"
      else
        pot_len = GettextpoHelper.get_pot_length(pot_path)
        @redis.hset('tphash_to_idrange', tphash, "#{@next_id} #{pot_len}")
        @next_id += pot_len
        @redis.set('next_id', @next_id)
      end
    end
  end

  def self.custom_basename(input)
    s = (input + '/').match(/(([a-zA-Z0-9\-\_\.%{}]+\/){3})$/)
    if s.nil? or s[1].nil?
      raise "path is too short to determine the basename: #{input}"
    end

    s[1][0..-2]
  end

  # List of SHA-1 hashes of new contents in a Git commit (for added or modified files)
  def self.contents_of_commit(repo_path, commit_sha1)
    parse_commit_changes(repo_path, commit_sha1).
      select {|x| x[4] != 'D' and x[5].match(/\.pot$/) }.
      map {|x| x[3] }
  end

  def load_known_broken_pots
    @redis.lrange('known_broken_pots', 0, -1)
  end

  def insert_known_broken_pot(pot_hash, pot_status, src_dir)
    @redis.lpush('known_broken_pots', pot_hash)
    @known_broken_pots << pot_hash

#    c_item = {
#      'type' => 'known_broken_pot',
#      'pot_hash' => pot_hash,
#      'broken_status' => pot_status,
#      'git_repo' => src_dir,
#    }
  end

  # Used for the IncrementalCommitProcessing object in add_templates_from_repo()
  class CouchbaseProcessedCommitsStorage
    def initialize(redis)
      @redis = redis
    end

    def list
      @redis.lrange('processed_templates_git_commits', 0, -1)
    end

    def add(commit_hash)
      @redis.lpush('processed_templates_git_commits', commit_hash)
    end
  end

  def add_templates_from_repo(src_dir)
    inc_proc = IncrementalCommitProcessing.new(src_dir, CouchbaseProcessedCommitsStorage.new(@redis))
    commits_to_process = inc_proc.commits_to_process

    tempfile_pot = Tempfile.new(['', '.pot']).path
    commits_to_process.each_with_index do |commit_sha1, index|
      puts ">>> Processing commit #{commit_sha1} (#{index + 1}/#{commits_to_process.size}) in #{src_dir}"
      contents = Sif.contents_of_commit(src_dir, commit_sha1)

      contents.each do |pot_hash|
        # pot_hash is the sha1 of blob (e.g. 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4)

        if @known_broken_pots.include?(pot_hash)
          puts "Repeated broken POT: #{pot_hash}"
          next
        end

        if have_info_for_pot_hash?(pot_hash)
#           puts "Already exists in the database: #{pot_hash}"
          next
        end

        puts "current file SHA1: #{pot_hash}"
        `cd #{src_dir} ; git show #{pot_hash} > "#{tempfile_pot}"`

        pot_status = is_virgin_pot(tempfile_pot)
        if pot_status == :ok
          add(tempfile_pot)
        else
          puts "Newly appeared broken POT: #{pot_hash}"

          insert_known_broken_pot(pot_hash, pot_status, src_dir)
        end
      end

      inc_proc.add_to_processed_list(commit_sha1)
    end

    `rm -f "#{tempfile_pot}"`
  end
end
