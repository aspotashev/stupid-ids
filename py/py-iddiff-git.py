#!/usr/bin/python3

# sudo dnf install python3-devel libgit2-devel
# sudo pip3 install pygit2==0.24
import json
import sys

import pygit2
import requests

from helpers.git_diffstat import GitCommitDiffStat, CommitFileChangeType
from helpers.iddiff import Iddiff, iddiff_files, iddiff_against_empty


class CalcserverClient(object):
    def __init__(self):
        pass

    def po_as_json(self, content):
        r = requests.put('http://localhost:9292/v1/po_as_json', {'content': content})
        return json.loads(json.loads(r.text)['template'])

    def get_tp_hash(self, content):
        r = requests.put('http://localhost:9292/v1/get_tp_hash', {'content': content})
        return json.loads(r.text)['tp_hash']


class TranslationContent(object):
    def __init__(self, git_object):
        assert type(git_object) == pygit2.Blob
        self.__po_content = git_object.data.decode('utf-8')

        template_json = self.__as_json()
        assert 'header' in template_json

        self.header = template_json['header']
        self.messages = template_json['messages']

        print(self.header)
        print(self.messages)

    def __as_json(self):
        return CalcserverClient().po_as_json(self.__po_content)

    def get_tp_hash(self):
        return CalcserverClient().get_tp_hash(self.__po_content)


if __name__ == "__main__":
    # a5420da8f86e7e5d5e670deb13833588b111380b - new files
    # cc447809d3248acc9f0382af32560472eef460b6 - changed files
    if len(sys.argv) != 3:
        print("Example usage:")
        print("  py-iddiff-git.py ./.git 7fb8df3aed979214165e7c7d28e672966b13a15b")
        sys.exit(1)

    _, repo_path, commit_oid = sys.argv

    repo = pygit2.Repository(repo_path)
    changes = GitCommitDiffStat(repo).iddiff_git(commit_oid)
    print(changes)

    res = Iddiff()
    for change in changes:
        if change.type == CommitFileChangeType.DEL:
            continue

        new_content = TranslationContent(repo.get(change.oid2))
        if change.type == CommitFileChangeType.ADD:
            res.merge(iddiff_against_empty(new_content))
        elif change.type == CommitFileChangeType.MOD:
            old_content = TranslationContent(repo.get(change.oid1))
            res.merge(iddiff_files(old_content, new_content))

    print(res.to_json())
