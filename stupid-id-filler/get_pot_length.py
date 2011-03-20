#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Prints the number of strings in a .po/.pot file.

import sys

from pology.catalog import Catalog
from pology.fsops import str_to_unicode


def _main ():

    path = str_to_unicode(sys.argv[1])
    cat = Catalog(path)
    print len(cat)


if __name__ == '__main__':
    _main()
