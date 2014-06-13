
class FillerBase < ActiveRecord::Base
	# http://pragdave.blogs.pragprog.com/pragdave/2006/01/sharing_externa.html
	self.abstract_class = true
	establish_connection_filler

	# 'data' should be a Hash
	def self.find_or_create(data)
		if where(data).count == 0
			create(data)
		end
	end
end

# [string] tp_hash <index> (template-part hash of a .pot)
# [string] potsha (Git hash of the .pot)
class TphashPotsha < FillerBase
end

# [string] tp_hash <index> (template-part hash of a .pot)
# [   int] first_id (ID of the first message in the .pot)
# [   int] id_count (number of messages in the .pot)
class TphashFirstId < FillerBase
  def self.get_pot_first_id(tp_hash)
    db_rows = find(:all, :conditions => {:tp_hash => tp_hash})

    if db_rows.size != 1
      # tp_hash not found in TphashFirstId (or duplicate rows)
      return nil
    end
    db_row = db_rows[0]

    [db_row.first_id, db_row.id_count]
  end
end

# last_sha1.value = sha1 of the last processed Git commit (string)
class FillerLastSha1 < FillerBase
	def self.value
		first.value
	end

	def self.value=(new_value)
		x = first
		x.value = new_value
		x.save!
	end
end

