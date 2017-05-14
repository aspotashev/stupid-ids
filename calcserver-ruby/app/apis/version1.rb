require 'grape'

require 'mongo'

$:.unshift(File.join(File.dirname(__FILE__)) + "../../../../build/gettextpo-helper/ruby-ext")
require 'stupidsruby'

Mongo::Logger.logger.level = ::Logger::INFO

def with_content
  content = params[:content]
  #p content

  tempfile_po = Tempfile.new(['', '.po']).path
  File.open(tempfile_po, 'w') do |file|
    file.write(content)
  end

  res = yield tempfile_po

  #p tempfile_po

  `rm -f "#{tempfile_po}"`
  res
end

class MongoClient
  def initialize
    mongo_client = Mongo::Client.new(['127.0.0.1:27017'], :database => 'stupids_db')
    @mongo = mongo_client[:template_parts]
  end

  def get_next_id
    # This is fast if you create an index in MongoDB:
    #   db.template_parts.createIndex({ first_id: -1 })
    last_item = @mongo.find({'$query': {}, '$orderby': {first_id: -1}}).limit(1).first
    if last_item.nil?
      100
    else
      last_item['first_id'] + last_item['pot_len']
    end
  end

  def tp_hash_exists?(tp_hash)
    @mongo.count(_id: tp_hash) > 0
  end

  def insert(tp_hash, next_id, pot_len, template_json)
    doc = {
        _id: tp_hash,
        first_id: next_id,
        pot_len: pot_len,
        template_json: template_json,
    }

    begin
      @mongo.insert_one(doc)
    rescue Mongo::Error::OperationFailure
      p doc
      raise
    end
  end
end

module MyApi
  class UserApiV1 < Grape::API
    version 'v1', :using => :path
    format :json

    put '/as_json' do
      with_content do |tempfile_po|
        { template: GettextpoHelper.file_template_as_json(tempfile_po) }
      end

      # pot_len = GettextpoHelper.get_pot_length(pot_path)
    end

    put '/po_as_json' do
      with_content do |tempfile_po|
        { template: GettextpoHelper.gettext_file_as_json(tempfile_po).force_encoding(Encoding::UTF_8) }
      end
    end

    put '/get_tp_hash' do
      with_content do |tempfile_po|
        { tp_hash: GettextpoHelper.calculate_tp_hash(tempfile_po) }
      end
    end

    post '/add_template' do
      mongo = MongoClient.new

      tp_hash = with_content do |tempfile_po|
        GettextpoHelper.calculate_tp_hash(tempfile_po)
      end

      if mongo.tp_hash_exists?(tp_hash)
        msg = "This template-part hash already exists in MongoDB: tp_hash = #{tp_hash}"
        puts msg

        error! msg, 422
      else
        pot_len, template_json = with_content do |tempfile_po|
          [GettextpoHelper.get_pot_length(tempfile_po), GettextpoHelper.file_template_as_json(tempfile_po)]
        end

        next_id = mongo.get_next_id
        mongo.insert(tp_hash, next_id, pot_len, template_json)

        { tp_hash: tp_hash }
      end
    end
  end
end
