
class FillerBase < ActiveRecord::Base
	# http://pragdave.blogs.pragprog.com/pragdave/2006/01/sharing_externa.html
	self.abstract_class = true
	establish_connection_filler
end

class NamedatePotsha < FillerBase
end

class PotshaFirstId < FillerBase
end

# last_sha1.value = sha1 of the last processed Git commit (string)
class FillerLastSha1 < FillerBase
	def self.value
		find(:first).value
	end

	def self.value=(new_value)
		x = find(:first)
		x.value = new_value
		x.save!
	end
end

