#!/usr/bin/ruby19

require '../ruby-helpers/ruby-helpers.rb'
require '../ruby-ext/gettextpo_helper'

require 'active_record'

ActiveRecord::Base.class_eval do
  def self.establish_connection_filler
    establish_connection(YAML::load(File.open('../../stupid-id-filler/database/filler-database.yml')))
  end
end

require '../../stupid-id-filler/database/filler-models.rb'



def git_commit_author(git_dir, git_ref)
  `cd #{git_dir} ; git log -1 --format=format:%an #{git_ref}`.strip
end

$TRANS_DIR = File.expand_path("~/kde-ru/kde-ru-trunk.git")
$DATABASE_DIR = "./mydb"

def read_po_buffer_messages(buffer)
  tempfile_pot = `tempfile --suffix=.pot`.strip
  File.open(tempfile_pot, 'w') do |f|
    f.write(buffer)
  end

  messages = GettextpoHelper.read_po_file_messages(tempfile_pot)
  `rm -f "#{tempfile_pot}"`

  messages
end

def calculate_tp_hash_buffer(buffer)
  tempfile_pot = `tempfile --suffix=.pot`.strip
  File.open(tempfile_pot, 'w') do |f|
    f.write(buffer)
  end

  tp_hash = GettextpoHelper.calculate_tp_hash(tempfile_pot)
  `rm -f "#{tempfile_pot}"`

  tp_hash
end

def read_po_sha1_messages(git_dir, git_ref)
  read_po_buffer_messages(`cd "#{git_dir}" ; git show #{git_ref}`)
end

def calculate_tp_hash_sha1(git_dir, git_ref)
  calculate_tp_hash_buffer(`cd "#{git_dir}" ; git show #{git_ref}`)
end

GettextpoHelper::Message.class_eval do
  def to_s
    "msgstr \"#{msgstr(0)}\""
  end
end

#$trans = {}

$msg_count = 0

def process_only_tr_changes(min_ids, messages_a, messages_b)
  raise if messages_a.size != messages_b.size

  (0...messages_a.size).each do |index|
    next if messages_a[index].equal_translations(messages_b[index])

#    ($trans[min_ids[index]] ||= []) << messages_b[index]
    puts min_ids[index].to_s.rjust(10) + ' ' + messages_b[index].to_s
  end
end

def process_new_tr_file(min_ids, messages)
  $msg_count += messages.size

  messages.each_with_index do |message, index|
    next if message.is_untranslated

#    ($trans[min_ids[index]] ||= []) << message
    puts min_ids[index].to_s.rjust(10) + ' ' + messages[index].to_s
  end
end

$id_map_db = GettextpoHelper::IdMapDb.new('../../successor-detector/database/idmap.mmapdb')

def get_min_ids_by_tp_hash(tp_hash)
  first_ids = TphashFirstId.get_pot_first_id(tp_hash)
  min_ids = nil
  if not first_ids.nil?
    return [] if first_ids[1] == 0

    min_ids = $id_map_db.get_min_id_array(*first_ids) # first_ids is a 2-element array
  end

  min_ids
end

inc_proc = IncrementalCommitProcessing.new($TRANS_DIR, $DATABASE_DIR)
commits = inc_proc.commits_to_process
commits.each_with_index do |sha1, index|
  puts "Processing #{sha1} (#{index + 1}/#{commits.size})"
  # TODO: we _probably_ shouldn't ignore changes by 'scripty', because if a human translator makes
  # a screwed up commit which is not readable, 'scripty' will then reveal the newly translated strings.
  if git_commit_author($TRANS_DIR, sha1) == 'scripty'
    inc_proc.add_to_processed_list(sha1)
    next
  end

  parse_commit_changes($TRANS_DIR, sha1).each do |x|
    if not x[5].match(/\.po$/)
      next
    end

    if x[4] == 'A'
      messages_b = read_po_sha1_messages($TRANS_DIR, x[3])
      tp_hash_b = calculate_tp_hash_sha1($TRANS_DIR, x[3])

      #min_ids = GettextpoHelper.get_min_ids_by_tp_hash(tp_hash_b) # this use stupids-server.rb TCP/IP server
      min_ids = get_min_ids_by_tp_hash(tp_hash_b)

      process_new_tr_file(min_ids, messages_b)
    elsif x[4] == 'M'
      messages_a = read_po_sha1_messages($TRANS_DIR, x[2])
      messages_b = read_po_sha1_messages($TRANS_DIR, x[3])

      tp_hash_a = calculate_tp_hash_sha1($TRANS_DIR, x[2])
      tp_hash_b = calculate_tp_hash_sha1($TRANS_DIR, x[3])
      # The translation templates do not necessarily match,
      # see http://websvn.kde.org/?view=revision&revision=815664

      #min_ids = GettextpoHelper.get_min_ids_by_tp_hash(tp_hash_b) # this use stupids-server.rb TCP/IP server
      min_ids = get_min_ids_by_tp_hash(tp_hash_b)

      if not min_ids.nil? # otherwise we cannot identify messages
        if tp_hash_a == tp_hash_b
          process_only_tr_changes(min_ids, messages_a, messages_b)
        else # tp_hash_a != tp_hash_b
          process_new_tr_file(min_ids, messages_b)
        end
      end
    end
  end

  puts "msg_count = #{$msg_count}"

#  inc_proc.add_to_processed_list(sha1)

#  $trans.keys.each do |min_id|
#    if $trans[min_id].size > 1
#      p $trans[min_id]
#      puts '---------------------------------'
#    end
#  end
end

