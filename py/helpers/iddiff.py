import json
import copy
from typing import Dict

from helpers.mongo_client import MongoClient


class IddiffHeader(object):
    def __init__(self, date, authors):
        self.date = [date] if date else []
        self.authors = authors or []

    def merge(self, that : 'IddiffHeader'):
        self.date.extend(that.date)
        self.authors = list(set(self.authors + that.authors))

    def to_json(self):
        return {
            'date': self.date,
            'authors': self.authors,
        }


class IddiffPerMsgid(object):
    def __init__(self, added=None, removed=None, review=None):
        self.added = added or []
        self.removed = removed or []
        self.review = review or []

    def __find_removed(self, msg):
        for iter in self.removed:
            if iter.msgstr == msg.msgstr:
                return iter

        return None

    def __find_added(self, msg):
        for iter in self.added:
            if iter.msgstr == msg.msgstr:
                return iter

        return None

    def __insert_removed(self, msg):
        if self.__find_removed(msg):
            # duplicate in "REMOVED"
            return

        if self.__find_added(msg):
            # conflict: trying to "REMOVE" a translation already existing in "ADDED"
            raise RuntimeError('trying to "REMOVE" a translation already existing in "ADDED"')

        self.removed.append(copy.deepcopy(msg))

    def __insert_added(self, msg):
        removed = self.__find_removed(msg)
        if removed:
            report = """
                Conflict: removing previously added translation
                  old message translation: %s
                  new message translation: %s
            """ % (str(removed), msg)
            raise RuntimeError(report)

        assert len(self.added) <= 1
        if len(self.added) == 0:
            self.added.append(msg)
        elif len(self.added) == 1:
            # Check for conflict: two different translations in "ADDED"
            assert(self.__find_added(msg))

    def __insert_review(self, msg):
        raise RuntimeError('not impl')

    def merge(self, that: 'IddiffPerMsgid'):
        for msg in that.removed:
            self.__insert_removed(msg)
        for msg in that.added:
            self.__insert_added(msg)
        for msg in that.review:
            self.__insert_review(msg)

    def __repr__(self):
        return json.dumps({
            'added': self.added,
            'removed': self.removed,
            'review': self.review,
        })


class Iddiff(object):
    def __init__(self, date=None, authors=None, per_msgid : Dict[int,IddiffPerMsgid]=None):
        self.header = IddiffHeader(date, authors)
        self.per_msgid = per_msgid or {}

    def merge(self, that: 'Iddiff'):
        self.header.merge(that.header)

        for msg_id, per_msgid in that.per_msgid.items():
            if msg_id in self.per_msgid:
                self.per_msgid[msg_id].merge(per_msgid)
            else:
                self.per_msgid[msg_id] = copy.deepcopy(per_msgid)

    def to_json(self):
        return {
            'header': self.header.to_json(),
            'per_msgid': self.per_msgid,
        }


def extract_msg_translation_only(msg):
    iddiff_msg = copy.deepcopy(msg)
    del iddiff_msg['formats']
    del iddiff_msg['untranslated']
    del iddiff_msg['filepos']
    del iddiff_msg['msgid']
    del iddiff_msg['comments']
    del iddiff_msg['obsolete']

    return iddiff_msg


# This function fills m_addedItems.
def iddiff_against_empty(content_b):
    tp_hash = content_b.get_tp_hash()
    first_id = MongoClient().get_first_id(tp_hash)
    assert first_id

    date = content_b.header['po_revision_date']
    authors = [content_b.header['last_translator']]
    per_msgid = {}

    for index, msg in enumerate(content_b.messages):
        # Messages can be:
        #     "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
        #     "abc" -- translated
        #     f"abc" -- fuzzy
        #
        # Types of possible changes:
        #     ""     -> ""     : -
        #     ""     -> "abc"  : ADDED
        #     ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)

        # Adding to "ADDED" if "B" is translated
        if msg['untranslated'] == False and msg['fuzzy'] == False:
            iddiff_msg = extract_msg_translation_only(msg)
            print(iddiff_msg)
            per_msgid[first_id + index] = IddiffPerMsgid(added=[iddiff_msg])

    return Iddiff(date, authors, per_msgid)


# This function fills m_removedItems and m_addedItems.
def iddiff_files(content_a, content_b):
    tp_hash = content_a.get_tp_hash()
    # .po files should be derived from the same .pot
    assert content_b.get_tp_hash() == tp_hash

    # first_id is the same for 2 files
    first_id = MongoClient().get_first_id(tp_hash)
    assert first_id

    date = content_b.header['po_revision_date']
    authors = [content_b.header['last_translator']]
    per_msgid = {}

    for index, pair in enumerate(zip(content_a.messages, content_b.messages)):
        msg_a, msg_b = pair

        # Messages can be:
        #   "" -- untranslated (does not matter fuzzy or not, so f"" is illegal)
        #   "abc" -- translated
        #   f"abc" -- fuzzy

        # Types of possible changes:
        need_added_removed = {
            # 00000    "abc"  -> "def"  : REMOVED, ADDED
            '00000': (True, True), # "abc" ->  "def"
            # 00001    ""     -> "abc"  : ADDED
            '00001': (True, False), # ""    ->  "abc"
            # 00010    "abc"  -> ""     : REMOVED
            '00010': (False, True), # "abc" ->  ""
            # 00011    ""     -> ""     : - (different nplurals)
            '00011': (False, False), # ""    ->  "" (different nplurals)
            # 00100    "abc"  -> "abc"  : -
            '00100': (False, False), # "abc" ->  "abc"
            # 00111    ""     -> ""     : -
            '00111': (False, False), # "" ->  ""
            # 01000    f"abc" -> "def"  : REMOVED, ADDED
            '01000': (True, True), # f"abc" ->  "def"
            # 01010    f"abc" -> ""     : REMOVED (removing fuzzy messages is OK)
            '01010': (False, True), # f"abc" ->  ""
            # 01100    f"abc" -> "abc"  : ADDED
            '01100': (True, False), # f"abc" ->  "abc"
            # 10000    "abc"  -> f"def" : REMOVED
            '10000': (False, True), # "abc" -> f"def"
            # 10001    ""     -> f"abc" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
            '10001': (False, False), # "" -> f"abc"
            # 10100    "abc"  -> f"abc" : REMOVED
            '10100': (False, True), # "abc" -> f"abc"
            # 11000    f"abc" -> f"def" : - (fuzzy messages are "weak", you should write in comments instead what you are not sure in)
            '11000': (False, False), # f"abc" -> f"def"
            # 11100    f"abc" -> f"abc" : -
            '11100': (False, False), # f"abc" -> f"abc"

            # no 00101 (empty cannot be equal to non-empty)
            # no 00110 (empty cannot be equal to non-empty)
            # no 01001 (empty+fuzzy A)
            # no 01011 (empty+fuzzy A, both empty but not equal)
            # no 01101 (empty+fuzzy A, empty cannot be equal to non-empty)
            # no 01110 (empty cannot be equal to non-empty)
            # no 01111 (empty+fuzzy A)
            # no 10010 (empty+fuzzy B)
            # no 10011 (empty+fuzzy B, both empty, but not equal)
            # no 10101   (empty cannot be equal to non-empty)
            # no 10110   (empty+fuzzy B, empty cannot be equal to non-empty)
            # no 10111   (empty+fuzzy B)
            # no 11001   (empty+fuzzy A)
            # no 11010   (empty+fuzzy B)
            # no 11011   (empty+fuzzy A, empty+fuzzy B, both empty, but not equal)
            # no 11101   (empty+fuzzy A, empty cannot be equal to non-empty)
            # no 11110   (empty+fuzzy B, empty cannot be equal to non-empty)
            # no 11111   (empty+fuzzy A, empty+fuzzy B)
        }

        # char 0 -- fuzzy B
        # char 1 -- fuzzy A
        # char 2 -- A == B (msgstr)
        # char 3 -- empty B
        # char 4 -- empty A
        key = ''
        key += str(1 if msg_b['fuzzy'] else 0)
        key += str(1 if msg_a['fuzzy'] else 0)
        key += str(1 if msg_a['msgstr'] == msg_b['msgstr'] else 0)
        key += str(1 if msg_b['untranslated'] else 0)
        key += str(1 if msg_a['untranslated'] else 0)

        need_added, need_removed = need_added_removed[key]

        per_msgid[first_id + index] = IddiffPerMsgid(
            removed=[extract_msg_translation_only(msg_a)] if need_added else [],
            added=[extract_msg_translation_only(msg_b)] if need_removed else [])

    return Iddiff(date, authors, per_msgid)
