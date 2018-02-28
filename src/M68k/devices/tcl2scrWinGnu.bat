sed -e '/^!/d' -e '/^$/d' -e 's/\\/\\\\/g' -e 's/\\$//g' -e 's/"/\\"/g' -e 's/^/"/' -e ': test' -e '/\\$/b slash' -e 's/$/\\n"/' -e 'p' -e 'd' -e ': slash' -e 'n' -e '/^!/d' -e '/^$/d' -e 's/"/\\"/g' -e 's/\\\\/\\/g' -e 's/\\n/\\\\n/g' -e 's/\\t/\\\\t/g' -e 's/\\f/\\\\f/g' -e 's/\\b/\\\\b/g' -e 'b test' < "%*"

