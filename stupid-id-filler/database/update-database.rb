#!/usr/bin/ruby19

require 'active_record'

ActiveRecord::Base.class_eval do
	def self.establish_connection_filler
		establish_connection(YAML::load(File.open('filler-database.yml')))
	end
end

require 'tempfile'
require './filler-models.rb'
require 'stupidsruby'
require '../../gettextpo-helper/ruby-helpers/ruby-helpers'

class CreateDb < ActiveRecord::Migration
	def self.connection
		FillerBase.connection
	end

	def self.up
		create_table FillerLastSha1.table_name do |t|
			t.string :value
		end

		create_table TphashPotsha.table_name do |t|
			t.string :tp_hash
			t.string :potsha
		end
		add_index TphashPotsha.table_name, [:tp_hash]
		add_index TphashPotsha.table_name, [:potsha]

		create_table TphashFirstId.table_name do |t|
			t.string :tp_hash
			t.integer :first_id
			t.integer :id_count
		end
		add_index TphashFirstId.table_name, [:tp_hash]

		FillerLastSha1.new(:id => 1, :value => 'init').save! # Git tag "init"
	end

	def self.down
		[FillerLastSha1, TphashPotsha, TphashFirstId].each do |c|
			drop_table c.table_name if table_exists?(c.table_name)
		end
	end
end

if not FillerLastSha1.table_exists?
	CreateDb.migrate(:down)
	CreateDb.migrate(:up)
end

$DIR = "../ids"

Array.class_eval do
	def is_uniq
		self.uniq.size == size
	end
end

# Differences between two sets
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
			elsif line.match(/^@@ /)
				# ok
			else
				p line
				p diff_lines
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

def git_diff_lines(ref1, ref2, filename)
	output = `cd "#{$DIR}" ; git diff -U0 -p #{ref1} #{ref2} -- "#{filename}"`.split("\n")
	if output.empty?
		return SetDiff.new # empty diff
	end

	if output[0][0...5] != 'diff ' or output[1][0...6] != 'index ' or output[2][0...4] != '--- ' or output[3][0...4] != '+++ '
		raise "wrong header in diff"
	end
	output = output[4..-1].select {|x| not x.match(/^@@ \-[0-9]+,[0-9]+ \+[0-9]+,[0-9]+ @@/) }

	SetDiff.new(output)
end

def get_id_count(tp_hash)
	tempfile_pot = Tempfile.new(['', '.pot']).path
	extract_pot_to_file(tp_hash, tempfile_pot)

	id_count = GettextpoHelper.get_pot_length(tempfile_pot)

	`rm -f "#{tempfile_pot}"`

	id_count
end

def update_database_to_git_commit(ref)
	# TODO: avoid duplicate code
	i = 1
	n = git_diff_lines(FillerLastSha1.value, ref, 'pot_origins.txt').added.size # TODO: avoid duplicate calls to git_diff_lines
	git_diff_lines(FillerLastSha1.value, ref, 'pot_origins.txt').added.each do |x|
		m = x.match(/^([0-9a-f]{40}) ([0-9a-f]{40})$/) or raise "failed to parse"
		TphashPotsha.find_or_create(:potsha => m[1], :tp_hash => m[2])

		if i % 37 == 12 or i == n
			print "\b"*30 + "Processing #{i}/#{n}"
			STDOUT.flush
		end
		i += 1
	end
	puts "    done!"

	i = 1
	n = git_diff_lines(FillerLastSha1.value, ref, 'first_ids.txt').added.size # TODO: avoid duplicate calls to git_diff_lines
	git_diff_lines(FillerLastSha1.value, ref, 'first_ids.txt').added.each do |x|
		m = x.match(/^([0-9a-f]{40}) ([0-9]+)$/) or raise "failed to parse"
		TphashFirstId.find_or_create(:tp_hash => m[1], :first_id => m[2].to_i, :id_count => get_id_count(m[1]))

		if i % 37 == 12 or i == n
			print "\b"*30 + "Processing #{i}/#{n}"
			STDOUT.flush
		end
		i += 1
	end
	puts "    done!"


	FillerLastSha1.value = ref
end


$NEW_SHA1 = git_head_sha1($DIR) # updating to this SHA-1

git_commits = git_commits_between($DIR, FillerLastSha1.value, $NEW_SHA1)
git_commits.each_with_index do |ref, index|
	puts "(#{index + 1}/#{git_commits.size})"
	update_database_to_git_commit(ref)
end

