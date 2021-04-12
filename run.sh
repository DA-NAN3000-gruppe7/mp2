#!/bin/bash

# Angir rotsti.
ROTFS=$PWD
INIT=$ROTFS/bin/init.sh

# Hvis bin-katalog ikke finnes, opprett dem.
if [ ! -d $ROTFS/bin ];then
    mkdir -p $ROTFS/bin
fi

# Hvis proc-katalog ikke finnes, opprett den.
if [ ! -d $ROTFS/proc ];then
    mkdir -p $ROTFS/proc
fi

# Hvis dumb-init ikke er i /bin, last ned.
if [ ! -f /bin/dumb-init ];then
    wget -O /bin/dumb-init https://github.com/Yelp/dumb-init/releases/download/v1.2.5/dumb-init_1.2.5_x86_64
    chmod +x /bin/dumb-init
fi

# Hvis det ikke er busybox sym-linker i bin, opprett dem.
if [[ -z $(find $ROTFS/bin -type l -ls) ]];then
    cd $ROTFS/bin/
    cp /bin/busybox .

    # Opprett symlinker til programmer i busybox.
    for P in $(./busybox --list); do
	ln -s busybox $P;
    done
fi

if gcc -o $ROTFS/bin/mp2.o $ROTFS/bin/mp2.c;then
    sudo chown root:root $ROTFS/bin/mp2.o
    sudo chmod u+s $ROTFS/bin/mp2.o
    
    # Mount proc som proc filsystem.
    sudo mount -t proc proc $ROTFS/proc

    # Flytt til rotkatalog.
    cd $ROTFS

    ls -l bin | grep -v busy
    echo $ROTFS

    # Lag namespace, endre rot og kjør init.sh
    sudo PATH=/bin unshare --fork --pid --mount-proc=$ROTFS/proc /usr/sbin/chroot . bin/init.sh

    # Unmount namespace.
    sudo umount $ROTFS/proc
fi
