#!/usr/bin/ruby18

require 'active_record'

ActiveRecord::Base.establish_connection(YAML::load(File.open('database.yml')))

class CreateDb < ActiveRecord::Migration
	def self.up
		create_table LastSha1.table_name do |t|
			t.string :value
		end

		create_table NamedatePotsha.table_name do |t|
			t.string :potdate
			t.string :potname

			t.string :potsha
		end
		add_index NamedatePotsha.table_name, [:potdate, :potname]

		LastSha1.create(:id => 1, :value => 'init').save! # Git tag "init"
	end

	def self.down
		[LastSha1, NamedatePotsha].each do |c|
			drop_table c.table_name if table_exists?(c.table_name)
		end
	end
end

class NamedatePotsha < ActiveRecord::Base
end

# last_sha1.value = sha1 of the last processed Git commit (string)
class LastSha1 < ActiveRecord::Base
	def self.value
		find(:first).value
	end

	def self.value=(new_value)
		x = find(:first)
		x.value = new_value
		x.save!
	end
end

if not LastSha1.table_exists?
	CreateDb.migrate(:down)
	CreateDb.migrate(:up)
end

$DIR = "../ids"
#$PREV_SHA1 = LastSha1.value
#LastSha1.value = LastSha1.value + '_xx'

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

def git_diff_lines(ref1, ref2)
	output = `cd "#{$DIR}" ; git diff -U0 -p #{ref1} #{ref2} -- pot_names.txt`.split("\n")
	if output.empty?
		return SetDiff.new # empty diff
	end

	if output[0][0...5] != 'diff ' or output[1][0...6] != 'index ' or output[2][0...4] != '--- ' or output[3][0...4] != '+++ '
		raise "wrong header in diff"
	end
	output = output[4..-1].select {|x| not x.match(/^@@ \-[0-9]+,[0-9]+ \+[0-9]+,[0-9]+ @@$/) }

	SetDiff.new(output)
end

def git_head_sha1
	`cd "#{$DIR}" ; git log --format=format:%H -1`.strip
end

$NEW_SHA1 = git_head_sha1 # updating to this SHA-1
set_diff = git_diff_lines(LastSha1.value, $NEW_SHA1)

set_diff.added.each do |x|
	m = x.match(/^([0-9a-f]{40}) ([^ ]+) <(.+)>$/)
	NamedatePotsha.create(:potsha => m[1], :potname => m[2], :potdate => m[3])
end

LastSha1.value = $NEW_SHA1


p NamedatePotsha.find(:all)

