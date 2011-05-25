#!/usr/bin/ruby19

require 'active_record'

ActiveRecord::Base.class_eval do
  def self.establish_connection_filler
    establish_connection(YAML::load(File.open('../stupid-id-filler/database/filler-database.yml')))
  end
end

require 'stupidsruby'
require '../stupid-id-filler/database/filler-models.rb'

def get_pot_first_id(tp_hash)
  TphashFirstId.find(:first, :conditions => {:tp_hash => tp_hash}).first_id
end

id_map_db = GettextpoHelper::IdMapDb.new('idmap.mmapdb')

def oid_to_tp_hash(sha1)
  # TODO: create a database index to make this work faster
  x = TphashPotsha.find(:first, :conditions => {:potsha => sha1})
  x.nil? ? nil : x.tp_hash
end


git_loader = GettextpoHelper::GitLoader.new
git_loader.add_repository("/home/sasha/kde-ru/xx-numbering/templates/.git/")
git_loader.add_repository("/home/sasha/kde-ru/xx-numbering/stable-templates/.git/")

pairs = GettextpoHelper.detect_transitions_inc(
  "/home/sasha/kde-ru/xx-numbering/templates/.git/",
  "/home/sasha/kde-ru/xx-numbering/stable-templates/.git/",
  "/home/sasha/l10n-kde4/scripts/process_orphans.txt",
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

