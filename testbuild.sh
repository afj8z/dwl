#!/bin/sh

[ -f config.h ] && mv config.h config.h.bak

# Generate a test config using an alternate modifier to avoid host interception
sed 's/WLR_MODIFIER_LOGO/WLR_MODIFIER_ALT/g' config.def.h > config.h

make clean dwl
mv dwl dwl-test

[ -f config.h.bak ] && mv config.h.bak config.h

# Launch nested session
./dwl-test -s 'foot'
