#!/bin/bash

git grep -l 'apples' | xargs sed -i 's/apples/oranges/g'
