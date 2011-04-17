#!/usr/bin/ruby19

require './gettextpo_helper'

p GettextpoHelper.calculate_tp_hash('/home/aspotashev/messages/kdebase/dolphin.po')
p GettextpoHelper.get_pot_length('/home/aspotashev/messages/kdebase/dolphin.po')

