#!/usr/bin/ruby

def lstree_output_to_pairs(input)
  input.split("\n").
    map {|x| x.match(/^[0-9]{6} blob ([0-9a-f]{40})\t(.+)$/) }.
    select {|x| not x.nil? }.
    map {|m| [m[1], m[2]] }.
    uniq.
    map {|x| [x[0], x[1].match(/(^|\/)([^\/]+)$/)[2]] }.
    uniq
end

def git_list_of_file_contents(dir)
  res = `cd #{dir} ; (for n in $(git log --format=format:%T) ; do git ls-tree -r --full-tree $n ; done) | sort | uniq`
  lstree_output_to_pairs(res)
end

p git_list_of_file_contents('~/kde-ru/xx-numbering/templates')

