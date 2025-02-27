#!/bin/sh
#
# Copyright (c) 2008, 2009 Edward Tomasz Napierała <trasz@FreeBSD.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
#

# This is a wrapper script to run tools-nfs4.test on ZFS filesystem.
#
# WARNING: It uses hardcoded ZFS pool name "acltools"
#
# If any of the tests fails, here is how to debug it: go to
# the directory with problematic filesystem mounted on it,
# and do /path/to/test run /path/to/test tools-nfs4.test, e.g.
#
# /usr/src/tools/regression/acltools/run /usr/src/tools/regression/acltools/tools-nfs4.test
#
# Output should be obvious.

if ! sysctl vfs.zfs.version.spa >/dev/null 2>&1; then
	echo "1..0 # SKIP system doesn't have ZFS loaded"
	exit 0
fi
if [ $(id -u) -ne 0 ]; then
	echo "1..0 # SKIP you must be root"
	exit 0
fi
if [ -z "$(which perl)" ]; then
	echo "1..0 # SKIP perl must be installed"
	exit 0
fi

echo "1..4"

TESTDIR=$(dirname $(realpath $0))

# Set up the test filesystem.
MD=`mdconfig -at swap -s 64m`
MNT=`mktemp -dt acltools`
trap "cd /; zpool destroy -f acltools; rmdir $MNT; mdconfig -d -u $MD" EXIT
zpool create -m $MNT acltools /dev/$MD
if [ $? -ne 0 ]; then
	echo "not ok 1 - 'zpool create' failed."
	echo 'Bail out!'
	exit 1
fi

echo "ok 1"

cd $MNT

# First, check whether we can crash the kernel by creating too many
# entries.  For some reason this won't work in the test file.
touch xxx
setfacl -x2 xxx
while :; do setfacl -a0 u:42:rwx:allow xxx 2> /dev/null; if [ $? -ne 0 ]; then break; fi; done
chmod 600 xxx
rm xxx
echo "ok 2"

perl $TESTDIR/run $TESTDIR/tools-nfs4-psarc.test >&2

if [ $? -eq 0 ]; then
	echo "ok 3"
else
	echo "not ok 3 # TODO: fails due to ACL changes in ZFS; bug 212323"
fi

echo "ok 4"
