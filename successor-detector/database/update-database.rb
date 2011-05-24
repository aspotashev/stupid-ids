#!/usr/bin/ruby19

require 'active_record'

ActiveRecord::Base.class_eval do
  def self.establish_connection_filler
    establish_connection(YAML::load(File.open('../../stupid-id-filler/database/filler-database.yml')))
  end
end

require 'stupidsruby'
require '../../gettextpo-helper/ruby-helpers/ruby-helpers'
require '../../stupid-id-filler/database/filler-models.rb'

Array.class_eval do
  def is_uniq
    self.uniq.size == size
  end
end

def get_pot_first_id(tp_hash)
  TphashFirstId.find(:first, :conditions => {:tp_hash => tp_hash}).first_id
end

$id_map_db = GettextpoHelper::IdMapDb.new('idmap.mmapdb')

$tphash_tempfile_pot = `tempfile --suffix=.pot`.strip
$tempfile_pot_a = `tempfile --suffix=.pot`.strip
$tempfile_pot_b = `tempfile --suffix=.pot`.strip

def oid_to_tp_hash(sha1)
  # TODO: create a database index to make this work faster
  x = TphashPotsha.find(:first, :conditions => {:potsha => sha1})
  x.nil? ? nil : x.tp_hash
end

def update_database
  pairs = GettextpoHelper.detect_transitions(
    "/home/sasha/kde-ru/xx-numbering/templates/.git/",
    "/home/sasha/kde-ru/xx-numbering/stable-templates/.git/",
    "/home/sasha/l10n-kde4/scripts/process_orphans.txt")
  pairs.each do |pair|
    tp_hash_a = oid_to_tp_hash(pair[0])
    tp_hash_b = oid_to_tp_hash(pair[1])
    next if tp_hash_a == tp_hash_b or tp_hash_a.nil? or tp_hash_b.nil?


    extract_pot_to_file(tp_hash_a, $tempfile_pot_a)
    extract_pot_to_file(tp_hash_b, $tempfile_pot_b)

    # Results should _probably_ be cached (in files or in a database)
    id_map_list = GettextpoHelper.list_equal_messages_ids_2(
      $tempfile_pot_a, get_pot_first_id(tp_hash_a),
      $tempfile_pot_b, get_pot_first_id(tp_hash_b))

    puts "#{tp_hash_a} <-> #{tp_hash_b}: #{id_map_list.size} pairs of IDs"

    id_map_list = id_map_list.
      map {|id_pair| id_pair.sort }.
      map {|id_pair| [id_pair[1], id_pair[0], 0] }

    raise if id_map_list.nil?
    raise if $id_map_db.nil?
    $id_map_db.create(id_map_list)
  end
end

update_database

$id_map_db.normalize_database

`rm -f "#{$tphash_tempfile_pot}"`
`rm -f "#{$tempfile_pot_a}"`
`rm -f "#{$tempfile_pot_b}"`

