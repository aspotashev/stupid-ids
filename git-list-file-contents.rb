#!/usr/bin/ruby

def git_list_of_file_contents(dir)
  res = `cd #{dir} ; (for n in $(git log --format=format:%T) ; do git ls-tree -r --full-tree $n ; done) | sort | uniq`
  res = res.split("\n").
    map {|x| x.match(/^[0-9]{6} blob ([0-9a-f]{40})\t(.+)$/) }.
    select {|x| not x.nil? }.
    map {|m| [m[1], m[2]] }.
    uniq.
    map {|x| [x[0], x[1].match(/(^|\/)([^\/]+)$/)[2]] }.
    uniq

## This is OK and often happens. For example, calligra/words.pot and koffice/kword.pot.
#  if res.size != res.map {|x| x[0] }.uniq.size
#    raise "Files with the same SHA-1s have different names."
#  end

  res
end

p git_list_of_file_contents('.')

