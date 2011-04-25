#!/usr/bin/ruby19

require 'active_record'

ActiveRecord::Base.class_eval do
	def self.establish_connection_filler
		establish_connection(YAML::load(File.open('../../stupid-id-filler/database/filler-database.yml')))
	end

	def self.establish_connection_merger
		establish_connection(YAML::load(File.open('merger-database.yml')))
	end
end

require '../../gettextpo-helper/ruby-ext/gettextpo_helper'
require '../../gettextpo-helper/ruby-helpers/ruby-helpers'
require '../../stupid-id-filler/database/filler-models.rb'
require './merger-models.rb'

class CreateMergerDb < ActiveRecord::Migration
	def self.connection
		MergerBase.connection
	end

	def self.up
		create_table MergerLastSha1.table_name do |t|
			t.string :value
		end
		MergerLastSha1.new(:id => 1, :value => 'init').save! # Git tag "init"

#		create_table IdMap.table_name do |t| # TODO: remove hidden attribute 'id'
#			t.integer :msg_id
#			t.integer :min_id
#			t.integer :merge_pair_id
#		end
#		add_index IdMap.table_name, [:msg_id]

		create_table MergePair.table_name do |t|
			t.string :tp_hash_a
			t.string :tp_hash_b
			t.integer :merge_file_id
		end

		create_table MergeFile.table_name do |t|
			t.string :subject
			t.string :author
			t.string :date
		end
	end

	def self.down
		[MergerLastSha1, MergePair, MergeFile].each do |c|
			drop_table c.table_name if table_exists?(c.table_name)
		end
	end
end

if not MergerLastSha1.table_exists?
	CreateMergerDb.migrate(:down)
	CreateMergerDb.migrate(:up)
end

$GIT_DIR = "../idmerges"

Array.class_eval do
	def is_uniq
		self.uniq.size == size
	end
end

class SetDiff < Struct.new(:added, :removed)
	def initialize(diff_lines = nil)
		super([], [])
		if diff_lines.nil?
			return
		end

		diff_lines.each do |line|
			if line[0..0] == '+'
				added << line[1..-1]
			elsif line[0..0] == '-'
				removed << line[1..-1]
			else
				raise "unexpected character (should be + or -)"
			end
		end

		check
	end

private
	def check
		raise "duplicated @added" if not added.is_uniq
		raise "duplicated @removed" if not removed.is_uniq
		raise "mixed commit; this is probably unusual" if added.size > 0 and removed.size > 0
		raise "removed strings; I thought we only add strings...; deletions are not handled!" if removed.size > 0
	end
end

def list_new_idmerges(ref1, ref2)
	output = `cd "#{$GIT_DIR}" ; git diff #{ref1} #{ref2} --raw --no-abbrev`.split("\n")
	lines = output.map {|x| x.match(/^:0{6} [0-9]{6} 0{40} ([0-9a-f]{40}) A\t.+\.idmerge$/) }
	lines.each do |m|
		raise "could not parse output" if m.nil? or m[1].size != 40
	end

	lines.map {|m| m[1] }
end

def get_pot_first_id(tp_hash)
	TphashFirstId.find(:first, :conditions => {:tp_hash => tp_hash}).first_id
end

$id_map_db = GettextpoHelper::IdMapDb.new('idmap.mmapdb')

def update_database_to_git_commit(ref)

new_idmerges = list_new_idmerges(MergerLastSha1.value, ref)
new_idmerges.each_with_index do |sha1, index|
	puts "Processing merge file #{sha1} (#{index + 1}/#{new_idmerges.size})..."

	content = `cd "#{$GIT_DIR}" ; git show #{sha1}`.split("\n")
	raise if not subject = content[0].match(/^ Subject: (.*)$/)[1]
	raise if not author  = content[1].match(/^ Author: (.*)$/)[1]
	raise if not date    = content[2].match(/^ Date: (.*)$/)[1]

	merge_file = MergeFile.create(:subject => subject, :author => author, :date => date)

	merges = content[3..-1].map {|x| x.match(/^([0-9a-f]{40}) ([0-9a-f]{40})$/) or raise "bug: #{x}" }
	merges.each do |m|
		tp_hash_a = m[1]
		tp_hash_b = m[2]
		raise if tp_hash_a == tp_hash_b

		merge_pair_data = { :merge_file_id => merge_file.id, :tp_hash_a => tp_hash_a, :tp_hash_b => tp_hash_b }
		merge_pair = MergePair.find(:first, :conditions => merge_pair_data)
		if merge_pair.nil?
			merge_pair = MergePair.create(merge_pair_data)
			merge_pair.save
		end

		tempfile_pot_a = `tempfile --suffix=.pot`.strip
		tempfile_pot_b = `tempfile --suffix=.pot`.strip

		extract_pot_to_file(tp_hash_a, tempfile_pot_a)
		extract_pot_to_file(tp_hash_b, tempfile_pot_b)

		id_map_list = GettextpoHelper.list_equal_messages_ids_2(
			tempfile_pot_a, get_pot_first_id(tp_hash_a),
			tempfile_pot_b, get_pot_first_id(tp_hash_b))

		`rm -f "#{tempfile_pot_a}"`
		`rm -f "#{tempfile_pot_b}"`

		puts "#{tp_hash_a} <-> #{tp_hash_b}: #{id_map_list.size} pairs of IDs"

		id_map_list = id_map_list.
			map {|id_pair| id_pair.sort }.
			map {|id_pair| [id_pair[1], id_pair[0], merge_pair.id] }

		raise if id_map_list.nil?
		raise if $id_map_db.nil?
		$id_map_db.create(id_map_list)
	end

end

MergerLastSha1.value = ref

end

$NEW_SHA1 = git_head_sha1($GIT_DIR) # updating to this SHA-1

git_commits = git_commits_between($GIT_DIR, MergerLastSha1.value, $NEW_SHA1)
git_commits.each_with_index do |ref, index|
	puts "(#{index + 1}/#{git_commits.size})"
	update_database_to_git_commit(ref)
end

$id_map_db.normalize_database

