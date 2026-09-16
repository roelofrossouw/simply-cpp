#!/bin/bash
#~~~~~~~~~~

server="${1:-build}"
user="${2:-root}"
scriptfile=$(realpath $0)
scriptpath="${scriptfile%/*}"
dirpath=$(realpath "$scriptpath"/..)
echo "Syncing $dirpath to $server:sc"
pushd $dirpath || exit
rsync -av ./ $user@$server:sc/ --exclude=".git" --exclude=".idea" --exclude="cmake-*" --delete
ssh $user@$server "./sc/scripts/run.sh"
popd || exit