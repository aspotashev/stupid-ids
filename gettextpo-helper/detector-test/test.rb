#!/usr/bin/ruby19

require 'stupidsruby'

pairs = GettextpoHelper.detect_transitions(
	"/home/sasha/kde-ru/xx-numbering/templates/.git/",
	"/home/sasha/kde-ru/xx-numbering/stable-templates/.git/",
	"/home/sasha/l10n-kde4/scripts/process_orphans.txt")

p pairs.size
p pairs[0..9]
p pairs.reverse[0..9]

