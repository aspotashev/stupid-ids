#!/usr/bin/ruby19

require '../../gettextpo-helper/ruby-ext/gettextpo_helper'

raise if ARGV.size != 1
GettextpoHelper::IdMapDb.new(ARGV[0]).normalize_database

