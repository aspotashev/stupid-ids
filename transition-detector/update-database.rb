#!/usr/bin/ruby19

$:.unshift(File.join(File.dirname(__FILE__)))
$:.unshift(File.join(File.dirname(__FILE__)) + "/../b/gettextpo-helper/ruby-ext")

require 'active_record'

ActiveRecord::Base.class_eval do
  def self.establish_connection_filler
    establish_connection(YAML::load(File.open('../stupid-id-filler/database/filler-database.yml')))
  end
end

require 'stupidsruby'
require '../stupid-id-filler/database/filler-models.rb'

$first_ids = GettextpoHelper::FiledbFirstIds.new(
  GettextpoHelper.stupids_conf_path("server.first_ids_db_path.first_ids"),
  GettextpoHelper.stupids_conf_path("server.first_ids_db_path.next_id"))

def get_pot_first_id(tp_hash)
  $first_ids.get_first_id(tp_hash).first
end

id_map_db = GettextpoHelper::IdMapDb.new('idmap.mmapdb')

def oid_to_tp_hash(sha1)
  # TODO: create a database index to make this work faster
  x = TphashPotsha.find(:first, :conditions => {:potsha => sha1})
  res1 = (x.nil? ? nil : x.tp_hash)

#  res2 = GettextpoHelper.tphash_cache(sha1)
#  raise "#{res1.inspect}  --vs--  #{res2.inspect}" if res1 != res2

  res1
end


git_loader = GettextpoHelper::GitLoader.new
git_loader.add_repository(GettextpoHelper.stupids_conf_path("generator.templates_git_repo.trunk"))
git_loader.add_repository(GettextpoHelper.stupids_conf_path("generator.templates_git_repo.stable"))

pairs = GettextpoHelper.detect_transitions_inc(
  GettextpoHelper.stupids_conf_path("generator.templates_git_repo.trunk"),
  GettextpoHelper.stupids_conf_path("generator.templates_git_repo.stable"),
  GettextpoHelper.stupids_conf_path("generator.process_orphans_txt"),
  "./processed-sha1-pairs.dat")

puts "Unprocessed pairs: #{pairs.size}"
pairs.each do |pair|
  tp_hash_a = oid_to_tp_hash(pair[0])
  tp_hash_b = oid_to_tp_hash(pair[1])
  next if tp_hash_a == tp_hash_b or tp_hash_a.nil? or tp_hash_b.nil?


  # Results should _probably_ be cached (in files or in a database)
  n_pairs = GettextpoHelper.dump_equal_messages_to_mmapdb(
    GettextpoHelper::TranslationContent.new(git_loader, pair[0]), get_pot_first_id(tp_hash_a),
    GettextpoHelper::TranslationContent.new(git_loader, pair[1]), get_pot_first_id(tp_hash_b),
    id_map_db)

  GettextpoHelper.append_processed_pairs("./processed-sha1-pairs.dat", [pair])

  puts "#{pair[0]} <-> #{pair[1]}: #{n_pairs} pairs of IDs"
end


id_map_db.normalize_database

