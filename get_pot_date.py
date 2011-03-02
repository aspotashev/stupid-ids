#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Prints the value of "POT-Creation-Date" of a .po/.pot file.

import sys

from pology.catalog import Catalog
from pology.fsops import str_to_unicode


def _main ():

    path = str_to_unicode(sys.argv[1])
    cat = Catalog(path)
    print cat.header.get_field_value("POT-Creation-Date")


if __name__ == '__main__':
    _main()

