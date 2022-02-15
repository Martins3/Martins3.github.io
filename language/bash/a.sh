#!/bin/bash

# git grep -l 'apples' | xargs sed -i 's/apples/oranges/g'

regex() {
    # Usage: regex "string" "regex"
    [[ $1 =~ $2 ]] && printf '%s\n' "${BASH_REMATCH[1]}"
}

regex "      hello" '^\s*(.*)'
