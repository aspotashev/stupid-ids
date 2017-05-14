require 'grape'

$:.unshift(File.join(File.dirname(__FILE__)) + "../../../../build/gettextpo-helper/ruby-ext")
require 'stupidsruby'

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
  end
end
