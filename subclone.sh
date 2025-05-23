#!/bin/bash

repo="$1"
repo_url="$2"
repo_branch="$3"

# Define the input file
INFILE="$repo/.gitmodules"

echo "Clone repo '$repo' from '$repo_url'"
rm -rf "$repo"
git clone "$repo_url" -b "$repo_branch" "$repo"
cd "$repo"
git gc --aggressive --prune=all
git remote add ce_private "git@github.com:CoreELEC/$repo-private.git"
cd ..

# Read the input file line by line
STEP="start"
while read -r LINE
do
    #echo "line: $LINE"
    case $STEP in
      "start")
        #printf '%s\n' "$LINE"
        if [[ $LINE == *"[submodule"* ]]; then
          submodule=$(echo "$LINE" | awk -F'"' '{print $2}')
          echo "submodule:" "$submodule"
          STEP="path"
        fi
        ;;
      "path")
        if [[ $LINE == *"path ="* ]]; then
          path=$(echo "$LINE" | awk -F' = ' '{print $2}')
          echo "path:" "$path"
          STEP="url"
        fi
        ;;
      "url")
        if [[ $LINE == *"url ="* ]]; then
          url=$(echo "$LINE" | awk -F' = ' '{print $2}')
          echo "url:" "$url"
          STEP="branch"
        fi
        ;;
      "branch")
        if [[ $LINE == *"branch ="* ]]; then
          branch=$(echo "$LINE" | awk -F' = ' '{print $2}')
          echo "branch:" "$branch"
          STEP="merge"
        fi
        ;;
    esac
    
    if [[ "$STEP" == "merge" ]]; then
        rm -rf "submodule"
        # step 1, clone
        echo "Clone $url to 'submodule'"
        depth=""
        if [[ $path == *"/bin" || $path == *"/build_system" || $path == *"/arch/riscv" ]]; then
          depth="--depth 1"
        fi
        git clone "$url" $depth --branch "$branch" --single-branch submodule
        # step 2, filter repo
        echo "Filter repo to subfolder '$path'"
        cd "submodule"
        git filter-repo \
          --to-subdirectory-filter "$path"
        echo "Garbage collection..."
        git gc --aggressive --prune=all
        cd ..
        # step 3, add remote
        echo "Add remote '$submodule'"
        cd "$repo"
        git remote add "$submodule" "../submodule"
        echo "Fetch remote '$submodule'"
        git fetch "$submodule"
        # step 4, clean up
        echo "Remove cached path '$path'"
        git rm --cached "$path"
        git commit -am "Removed cached path '$path' for '$submodule'"
        # step 5, merge
        echo "Merge '$submodule'"
        git merge --allow-unrelated-histories "$submodule/$branch"
        #read -p "Fix merge errors if any..." < /dev/tty
        git remote remove "$submodule"
        cd ..
        rm -rf submodule
        echo "Submodule '$submodule' is done"
        echo ""
        STEP="start"
    fi
done < "$INFILE"
