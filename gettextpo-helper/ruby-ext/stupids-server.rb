#!/usr/bin/ruby19
# Ruby 1.9 is required for gettextpo_helper Ruby extension

require './gettextpo_helper'

require 'active_record'

ActiveRecord::Base.class_eval do
	def self.establish_connection_filler
		establish_connection(YAML::load(File.open('../../stupid-id-filler/database/filler-database.yml')))
	end
end

require '../../stupid-id-filler/database/filler-models.rb'


require 'gserver'

$id_map_db = GettextpoHelper::IdMapDb.new('../../successor-detector/database/idmap.mmapdb')

# http://www.rubyinside.com/advent2006/10-gserver.html
# http://www.ruby-doc.org/stdlib/libdoc/gserver/rdoc/index.html
class StupidsServer < GServer
	def serve(io)
		loop do
			line = io.readline.strip

			space_index = line.index(' ')
			command, args = space_index ? [line[0..(space_index - 1)], line[(space_index + 1)..-1]] : [line, nil]

			if command == 'get_min_id_array' # args: tp_hash
				if !args or !args.match(/^[0-9a-f]{40}$/)
					io.puts "get_min_id_array:  invalid arguments"
					return
				end


				db_rows = TphashFirstId.find(:all, :conditions => {:tp_hash => args})

				if db_rows.size != 1
					io.puts "NOTFOUND" # tp_hash not found in TphashFirstId (or duplicate rows)
					return
				end
				db_row = db_rows[0]

				first_id = db_row.first_id
				id_count = db_row.id_count

				begin
					id_array = $id_map_db.get_min_id_array(first_id, id_count)
				rescue => e
					p e
				end

				if id_array.size != id_count
					io.puts "id_array.size != id_count"
					return
				end

				io.puts id_array.size
				io.puts id_array
			else
				io.puts "Unknown command: " + [command, args].inspect
			end
		end
	end
end

server = StupidsServer.new(1234)
server.start
server.join
