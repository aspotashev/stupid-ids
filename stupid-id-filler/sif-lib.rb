
require '../gettextpo-helper/ruby-ext/gettextpo_helper'

class Sif
	# http://www.oreillynet.com/ruby/blog/2007/01/nubygems_dont_use_class_variab_1.html
	@object_created = false
	class << self; attr_accessor :object_created; end

	def next_id_txt     ; @ids_dir + '/next_id.txt'     ; end
	def first_ids_txt   ; @ids_dir + '/first_ids.txt'   ; end
	def pot_origins_txt ; @ids_dir + '/pot_origins.txt' ; end

	def initialize(ids_dir)
		if self.class.object_created
			raise "can create only one instance of class Sif (to prevent conflicting changes in files)"
		else
			self.class.object_created = true
		end

		@ids_dir = ids_dir

		`cd #{@ids_dir} ; git reset --hard` # reset not commited changes

		@next_id = IO.read(next_id_txt).strip.to_i

		@commited_tp_hashes = IO.read(first_ids_txt).split("\n").map {|x| x[0...40] }
		@commited_sha1s = IO.read(pot_origins_txt).split("\n").map {|x| x[0...40] }

		@new_firstids = [] # pairs: (tp_hash, first_id)
		@new_origins  = [] # pairs: (sha1, tp_hash)

		@dirty = false
	end

	def add(pot_path, options = {})
		pot_hash = options[:pot_hash] || `git-hash-object "#{pot_path}"`.strip

		# Prepare new data for pot_origins.txt
		if @commited_sha1s.include?(pot_hash) or @new_origins.map(&:first).include?(pot_hash)
			puts "This .pot hash already exists in pot_origins.txt (or already scheduled for addition)"
		else
			@dirty = true

			tp_hash = GettextpoHelper.calculate_tp_hash(pot_path)
			@new_origins << [pot_hash, tp_hash]

			# Prepare new data for first_ids.txt
			if @commited_tp_hashes.include?(tp_hash) or @new_firstids.map(&:first).include?(tp_hash)
				puts "This template-part hash already exists in first_ids.txt (or already scheduled for addition)"
			else
				pot_len = GettextpoHelper.get_pot_length(pot_path)

				@new_firstids << [tp_hash, @next_id]
				@next_id += pot_len
			end
		end
	end

	def commit
		return if @dirty == false # nothing has changed

		# write new data to files
		File.open(first_ids_txt, 'a+') do |f|
			f.puts @new_firstids.map {|x| x.join(' ') }
		end

		File.open(pot_origins_txt, 'a+') do |f|
			f.puts @new_origins.map {|x| x.join(' ') }
		end

		File.open(next_id_txt, 'w') do |f|
			f.print @next_id
		end

		# commit
		puts `cd "#{@ids_dir}" ; git commit -m upd -- first_ids.txt next_id.txt pot_origins.txt`

		# data is already committed
		@commited_tp_hashes.concat(@new_firstids.map(&:first))
		@new_firstids = []

		@commited_sha1s.concat(@new_origins.map(&:first))
		@new_origins = []

		@dirty = false
	end
end

