
$:.unshift(File.join(File.dirname(__FILE__)) + "/../build/gettextpo-helper/ruby-ext")

require 'stupidsruby'
require 'set'
require 'securerandom'
require 'couchbase'

class Sif
  # http://www.oreillynet.com/ruby/blog/2007/01/nubygems_dont_use_class_variab_1.html
  @object_created = false
  class << self; attr_accessor :object_created; end

  def initialize(ids_dir)
    if self.class.object_created
      raise "can create only one instance of class Sif (to prevent conflicting changes in files)"
    else
      self.class.object_created = true
    end

    @conn = Couchbase.connect(bucket: 'stupids')

    begin
      @conn.add('next_id', 100)
    rescue Couchbase::Error::KeyExists
    end
    @next_id = @conn.get('next_id')

    @conn.save_design_doc(File.open(File.join(File.dirname(__FILE__)) + '/stupids-couchbase.json'))
    @views = @conn.design_docs['stupids-couchbase']
  end

  def self.git_hash_object(path)
    data = "blob #{File.size(path).to_s(10)}\0" + File.open(path).read
    Digest::SHA1.hexdigest(data)
  end

  def add(pot_path)
    pot_hash = Sif.git_hash_object(pot_path)

    tphash_view = @views.tp_hash_by_pot(key: pot_hash)

    if tphash_view.count > 0
      #puts "This .pot hash already exists in Couchbase"
    else
      tp_hash = GettextpoHelper.calculate_tp_hash(pot_path)
      raise if tp_hash.size != 40

      id = SecureRandom.urlsafe_base64(16)
      item = {
        'type' => 'tp_hash',
        'pot_hash' => pot_hash,
        'tp_hash' => tp_hash,
      }
      @conn.add(id, item)

      id_view = @views.first_id_by_tp_hash(key: pot_hash)

      if id_view.count > 0
        puts "This template-part hash already exists in Couchbase"
      else
        pot_len = GettextpoHelper.get_pot_length(pot_path)

        id = SecureRandom.urlsafe_base64(16)
        item = {
          'type' => 'first_id',
          'tp_hash' => tp_hash,
          'first_id' => @next_id,
          'pot_len' => pot_len,
        }
        @conn.add(id, item)

        @next_id += pot_len
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

    tempfile_pot = Tempfile.new(['', '.pot']).path
    commits_to_process.each_with_index do |commit_sha1, index|
      puts ">>> Processing commit #{commit_sha1} (#{index + 1}/#{commits_to_process.size}) in #{src_dir}"
      contents = Sif.contents_of_commit(src_dir, commit_sha1)
      broken_in_commit = []

      contents.each do |content_sha1|
        # content_sha1 is the sha1 of blob (e.g. 963a86ab2a7f24ba4400eace2e713c5bb8a5bad4)

        puts "current file SHA1: #{content_sha1}"
        `cd #{src_dir} ; git show #{content_sha1} > "#{tempfile_pot}"`

        pot_status = is_virgin_pot(tempfile_pot)
        if pot_status == :ok
          add(tempfile_pot)
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
end
