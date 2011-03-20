
class Sif
	# http://www.oreillynet.com/ruby/blog/2007/01/nubygems_dont_use_class_variab_1.html
	@object_created = false
	class << self; attr_accessor :object_created; end

	def next_id_txt   ; @ids_dir + '/next_id.txt'   ; end
	def first_ids_txt ; @ids_dir + '/first_ids.txt' ; end
	def pot_names_txt ; @ids_dir + '/pot_names.txt' ; end

	def initialize(ids_dir)
		if self.class.object_created
			raise "can create only one instance of class Sif (to prevent conflicting changes in files)"
		else
			self.class.object_created = true
		end

		@ids_dir = ids_dir

		`cd #{@ids_dir} ; git reset --hard` # reset not commited changes

		@next_id = File.open(next_id_txt).read.strip.to_i

		@commited_sha1s = File.open(first_ids_txt).read.split("\n").map {|x| x[0...40] }
		@commited_potnames = File.open(pot_names_txt).read.split("\n")

		@new_firstids = [] # pairs: (sha1, first_id)
		@new_potnames = [] # strings like in pot_names.txt: "$POT_HASH $POT_NAME <$POT_DATE>"
	end

	def add(pot_path, pot_name)
		pot_hash = `git-hash-object "#{pot_path}"`.strip # TODO: the caller code might already know the hash, no need to recalculate it then
		pot_date = `python ./get_pot_date.py #{pot_path}`.strip

		new_potnames_line = "#{pot_hash} #{pot_name} <#{pot_date}>"
		if @commited_potnames.include?(new_potnames_line) or @new_potnames.include?(new_potnames_line)
			puts "This line already exists in pot_names.txt (or already scheduled for addition)"
		else
			@new_potnames << new_potnames_line
		end

		if @commited_sha1s.include?(pot_hash) or @new_firstids.map(&:first).include?(pot_hash)
			puts "This SHA-1 already exists in first_ids.txt (or already scheduled for addition)"
		else
			pot_len = `python ./get_pot_length.py "#{pot_path}"`.strip.to_i

			@new_firstids << [pot_hash, @next_id]
			@next_id += pot_len
		end
	end

	def commit
		# write new data to files
		File.open(pot_names_txt, 'a+') do |f|
			f.puts @new_potnames
		end

		File.open(first_ids_txt, 'a+') do |f|
			f.puts @new_firstids.map {|x| x.join(' ') }
		end

		File.open(next_id_txt, 'w') do |f|
			f.print @next_id
		end

		# commit
		puts `cd "#{@ids_dir}" ; git commit -m upd -- first_ids.txt next_id.txt pot_names.txt`

		# data is already committed
		@commited_sha1s.concat(@new_firstids.map(&:first))
		@new_firstids = []

		@commited_potnames.concat(@new_potnames)
		@new_potnames = []
	end
end

