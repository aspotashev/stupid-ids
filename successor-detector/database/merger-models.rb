
class MergerBase < ActiveRecord::Base
	# http://pragdave.blogs.pragprog.com/pragdave/2006/01/sharing_externa.html
	self.abstract_class = true
	establish_connection_merger
end

# [   int] msg_id
# [   int] min_id
# [   int] merge_pair_id
#class IdMap < MergerBase
#	validates_uniqueness_of :msg_id
#end

# [   int] id (ActiveRecord creates it automatically)
# [string] tp_hash_a
# [string] tp_hash_b
# [   int] merge_file_id
class MergePair < MergerBase
	# TODO: how to simplify this?
	validates_uniqueness_of :merge_file_id, :scope => [:tp_hash_a, :tp_hash_b]
	validates_uniqueness_of :tp_hash_b, :scope => [:merge_file_id, :tp_hash_a]
	validates_uniqueness_of :tp_hash_a, :scope => [:tp_hash_b, :merge_file_id]
end

# [   int] id (ActiveRecord creates it automatically)
# [string] subject ('Subject:' field from .idmerge file)
# [string] author ('Author:' field from .idmerge file)
# [string] date ('Date:' field from .idmerge file)
class MergeFile < MergerBase
	# TODO: how to simplify this?
	validates_uniqueness_of :subject, :scope => [:author, :date]
	validates_uniqueness_of :author, :scope => [:subject, :date]
	validates_uniqueness_of :date, :scope => [:subject, :author]
end

# last_sha1.value = sha1 of the last processed Git commit (string)
class MergerLastSha1 < MergerBase
	def self.value
		find(:first).value
	end

	def self.value=(new_value)
		x = find(:first)
		x.value = new_value
		x.save!
	end
end

