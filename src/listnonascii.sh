#!/bin/bash

find . \( -name "*.cpp" -or -name "*.h" \) -exec grep --color='auto' -P -Hn "[\x80-\xFF]" {} \;
