#!/bin/bash
 
#UNFINISHED SCRIPT!!!!!!!
#NO DOCUMENTATION YET!

ROTFS=$PWD
INIT=init.sh


cd       $ROTFS/bin/
cp       /bin/busybox .
for P in $(./busybox --list); do ln busybox $P; done;



cd $ROTFS
sudo PATH=/bin unshare --fork --pid /usr/sbin/chroot . $ROTFS/bin/init.sh