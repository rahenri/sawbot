#/bin/sh

OUTPUT=submit.zip

rm -rf ${OUTPUT}

FILES=$(find .  | grep '\.\(h\|cpp\)$' | grep -v '_test\.cpp$')

zip ${OUTPUT} ${FILES}
