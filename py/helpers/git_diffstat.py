from enum import Enum

import pygit2


class CommitFileChangeType(Enum):
    DEL = 1
    ADD = 2
    MOD = 3


class CommitFileChange(object):
    def __init__(self, oid1, oid2, path, name, type):
        self.oid1 = oid1
        self.oid2 = oid2
        self.path = path
        self.name = name
        self.type = type

    def __repr__(self):
        return self.name


class GitCommitDiffStat(object):
    def __init__(self, repo):
        self.__repo = repo

    @staticmethod
    def __check_lexicographical_order(names):
        for i in range(len(names) - 1):
            assert names[i] < names[i + 1],\
                "i = %d, names[i] = %s, names[i + 1] = %s, names = %s" % (i, names[i], names[i + 1], str(names))

    def __diff_tree(self, tree1, tree2, path):
        if (tree1 is not None) and (tree2 is not None) and tree1.id == tree2.id:
            raise RuntimeError('Empty commit')

        # NOTE: the trees will be read twice for every commit, because
        # almost every commit is a parent and is a child at the same time.

        res = []

        it1 = iter(tree1)
        it2 = iter(tree2)

        tree1_names = []
        tree2_names = []
        entry1 = next(it1)
        entry2 = next(it2)
        while it1 is not None or it2 is not None:
            if it1 and (len(tree1_names) == 0 or entry1.name != tree1_names[-1]):
                tree1_names.append(entry1.name)
            if it2 and (len(tree2_names) == 0 or entry2.name != tree2_names[-1]):
                tree2_names.append(entry2.name)

            cmp = 0
            if it1 and it2:
                if entry1.name < entry2.name:
                    cmp = -1
                elif entry1.name > entry2.name:
                    cmp = 1

            if it2 is None or cmp < 0:
                # entry1 goes first, i.e. the entry is being removed in this commit
                name = entry1.name

                if entry1.filemode & pygit2.GIT_FILEMODE_TREE:
                    subtree = self.__repo.get(entry1.id)
                    subtree_path = path + '/' + name
                    res.extend(self.__diff_tree(subtree, None, subtree_path))
                else:
                    res.append(CommitFileChange(entry1.id, None, path, name, CommitFileChangeType.DEL))

                entry1 = next(it1)
            elif it1 is None or cmp > 0:
                # entry2 goes first, i.e. the entry is being added in this commit
                name = entry2.name

                if entry2.filemode & pygit2.GIT_FILEMODE_TREE:
                    subtree = self.__repo.get(entry2.id)
                    subtree_path = path + '/' + name
                    res.extend(self.__diff_tree(None, subtree, subtree_path))
                else:
                    res.append(CommitFileChange(None, entry2.id, path, name, CommitFileChangeType.ADD))

                entry2 = next(it2)
            else:
                # both entries exist and have the same names, i.e. the entry is being modified in this commit
                if entry1.id == entry2.id:
                    # Entry unchanged
                    pass
                else:
                    attr1 = entry1.filemode
                    attr2 = entry2.filemode
                    if attr1 != attr2 and not (attr1 == 0o100755 and attr2 == 0o100644):
                        raise RuntimeError("Git tree entry attributes are changing in this commit")

                    name = entry1.name
                    if attr1 & pygit2.GIT_FILEMODE_TREE:
                        subtree1 = self.__repo.get(entry1.id)
                        subtree2 = self.__repo.get(entry2.id)
                        subtree_path = path + '/' + name
                        res.extend(self.__diff_tree(subtree1, subtree2, subtree_path))
                    else:
                        res.append(CommitFileChange(entry1.id, entry2.id, path, name, CommitFileChangeType.MOD))

                try:
                    entry1 = next(it1)
                except StopIteration:
                    it1 = None

                try:
                    entry2 = next(it2)
                except StopIteration:
                    it2 = None

        # We assume that tree entries are sorted by name
        self.__check_lexicographical_order(tree1_names)
        self.__check_lexicographical_order(tree2_names)

        return res

    # commit1 -- parent commit
    # commit2 -- current commit
    def __diff_commit(self, commit1, commit2):
        return self.__diff_tree(commit1.tree, commit2.tree, '')

    def iddiff_git(self, commit_oid):
        commit = self.__repo.get(commit_oid)

        assert commit is not None
        assert type(commit) == pygit2.Commit

        if len(commit.parents) == 0:
            parent = None
        elif len(commit.parents) == 1:
            parent = commit.parents[0]
        else:
            raise RuntimeError("More than one parent")

        return self.__diff_commit(parent, commit)
