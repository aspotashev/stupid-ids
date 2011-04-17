#!/usr/bin/ruby19

require './gettextpo_helper'

p GettextpoHelper.calculate_tp_hash(File.expand_path('~/messages/kdebase/dolphin.po'))
p GettextpoHelper.get_pot_length(File.expand_path('~/messages/kdebase/dolphin.po'))

id_map_db = GettextpoHelper::IdMapDb.new('idmap.mmapdb')
id_map_db.create([[12345, 100, 5]]) # msg_id, min_id, merge_pair_id

p id_map_db.get_min_id(50000)
p id_map_db.get_min_id(12345)
id_map_db.normalize_database

