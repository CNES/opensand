#!/bin/sh

## live-build(7) - System Build Scripts
## Copyright (C) 2006-2012 Daniel Baumann <daniel@debian.org>
##
## live-build comes with ABSOLUTELY NO WARRANTY; for details see COPYING.
## This is free software, and you are welcome to redistribute it
## under certain conditions; see COPYING for details.


set -e

# Removing unused packages
for PACKAGE in apt-utils aptitude man-db manpages info wget
do
	if ! apt-get remove --purge --yes "${PACKAGE}"
	then
		echo "WARNING: ${PACKAGE} isn't installed"
	fi
done

apt-get autoremove --yes || true

# Removing unused files
find . -name *~ -print0 | xargs -0 rm -f

rm -rf /var/cache/man/*

# Truncating logs
for FILE in $(find /var/log/ -type f)
do
	: > ${FILE}
done
