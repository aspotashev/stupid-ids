import pymongo


class MongoClient(object):
    def __init__(self):
        connection = pymongo.MongoClient()
        self.__db = connection['stupids_db']['template_parts']

    def get_first_id(self, tp_hash):
        rows = list(self.__db.find({'_id': tp_hash}))
        assert len(rows) <= 1

        if len(rows) == 0:
            return None
        elif len(rows) == 1:
            return rows[0]['first_id']
