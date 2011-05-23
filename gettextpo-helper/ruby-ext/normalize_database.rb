#!/usr/bin/ruby19

require 'stupidsruby'

raise if ARGV.size != 1
GettextpoHelper::IdMapDb.new(ARGV[0]).normalize_database

