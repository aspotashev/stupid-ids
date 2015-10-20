
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
    `git hash-object "#{path}"`.strip
  end

  def add(pot_path, options = {})
    pot_hash = options[:pot_hash] || git_hash_object(pot_path)

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
end
