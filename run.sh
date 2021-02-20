#!/bin/bash
 

#assings path to current directory and to init.sh
ROTFS=$PWD
INIT=$ROTFS/bin/init.sh


#if dumb-init is not in bin, download it
if [ ! -f $ROTFS/bin/dumb-init ];then
	 wget -O $ROTFS/bin/dumb-init https://github.com/Yelp/dumb-init/releases/download/v1.2.5/dumb-init_1.2.5_x86_64
	 chmod +x $ROTFS/bin/dumb-init
fi
#if there are no busybox sym-links, create them
if [[ -z $(find $ROTFS/bin -type l -ls) ]];then
	cd $ROTFS/bin/
	cp /bin/busybox .

	for P in $(./busybox --list); do 
		ln -s busybox $P; 
	done
fi

#compile the webserver.c code
#adding a condition so that the server doesn't run if webserver.c failed compiling
if gcc $ROTFS/bin/webserver.c --static -o $ROTFS/bin/webserver.o; then
	sudo chown root:root $ROTFS/bin/webserver.o
	sudo chmod u+s $ROTFS/bin/webserver.o

	cd $ROTFS

	#mount /proc as proc in current directory
	sudo mount -t proc proc $ROTFS/proc

	#create new namespace and change root, executing init.sh
	sudo PATH=/bin unshare -p -f --mount-proc=$ROTFS/proc /usr/sbin/chroot . /bin/init.sh

	#unmounting the namespace
	sudo umount $ROTFS/proc
fi