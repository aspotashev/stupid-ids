require 'grape'

$:.unshift(File.join(File.dirname(__FILE__)) + "../../../../build/gettextpo-helper/ruby-ext")
require 'stupidsruby'

module MyApi
  class UserApiV1 < Grape::API
    version 'v1', :using => :path
    format :json

    put '/as_json' do
      content = params[:content]
      #p content

      # tphash = GettextpoHelper.calculate_tp_hash(pot_path)
      # pot_len = GettextpoHelper.get_pot_length(pot_path)

      tempfile_pot = Tempfile.new(['', '.pot']).path
      File.open(tempfile_pot, 'w') do |file|
        file.write(content)
      end

      res = {
          template: GettextpoHelper.file_template_as_json(tempfile_pot),
      }
      #p tempfile_pot

      `rm -f "#{tempfile_pot}"`

      res
    end

    put '/po_as_json' do
      content = params[:content]
      #p content

      # tphash = GettextpoHelper.calculate_tp_hash(pot_path)
      # pot_len = GettextpoHelper.get_pot_length(pot_path)

      tempfile_pot = Tempfile.new(['', '.po']).path
      File.open(tempfile_pot, 'w') do |file|
        file.write(content)
      end

      res = {
          template: GettextpoHelper.gettext_file_as_json(tempfile_pot).force_encoding(Encoding::UTF_8),
      }
      #p tempfile_pot

      `rm -f "#{tempfile_pot}"`

      res
    end
  end
end
