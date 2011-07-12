#!/bin/sh

TEMPLATES="~/kde-ru/xx-numbering/templates"
TEMPLATES_STABLE="~/kde-ru/xx-numbering/stable-templates"

bash -c "cd $TEMPLATES        && git reset --hard && git svn fetch && git rebase git-svn"
bash -c "cd $TEMPLATES_STABLE && git reset --hard && git svn fetch && git rebase git-svn"

bash -c "cd stupid-id-filler && ./add-templates-from-repo.rb $TEMPLATES ./ids"
bash -c "cd stupid-id-filler && ./add-templates-from-repo.rb $TEMPLATES_STABLE ./ids"

bash -c "cd stupid-id-filler/database && ./update-database.rb"

bash -c "cd transition-detector && ./update-database.rb"

