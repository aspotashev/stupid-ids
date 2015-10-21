#!/usr/bin/ruby20

require '../sif-lib.rb'

if Sif.git_hash_object(File.join(File.dirname(__FILE__)) + '/../../autotests/data/tp_hash/base.po') != '93d7a8e4eeb340643da7161f2f2f2ef3d195a780'
  raise 'FAIL'
else
  puts 'OK'
end
