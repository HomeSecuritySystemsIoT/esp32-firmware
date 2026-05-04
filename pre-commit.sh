#!/usr/bin/env bash

# pre-commit hook to format commited c/cpp/h files with clang-format copy to .git/hooks/pre-commit and make it executable

set -e

files=$(git diff --cached --name-only --diff-filter=ACMR | grep -E '\.(c|cpp|h)$' || true)

[ -z "$files" ] && exit 0

for file in $files; do
  clang-format -i -style=file:ESP32/.clang-format "$file"
  git add "$file"
done
