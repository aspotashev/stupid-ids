
class FillerBase < ActiveRecord::Base
	# http://pragdave.blogs.pragprog.com/pragdave/2006/01/sharing_externa.html
	self.abstract_class = true
	establish_connection_filler
end

# [string] tp_hash <index> (template-part hash of a .pot)
# [string] potsha (Git hash of the .pot)
class TphashPotsha < FillerBase
end

# [string] tp_hash <index> (template-part hash of a .pot)
# [   int] first_id (ID of the first message in the .pot)
# [   int] id_count (number of messages in the .pot)
class TphashFirstId < FillerBase
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

