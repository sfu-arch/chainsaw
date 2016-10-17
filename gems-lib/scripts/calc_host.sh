#!/bin/bash -f

export OS=`uname -s`
export HOST_ARCH=`uname -m`

case ${OS} in
    "Linux")
		export OS_PART=linux
        ;;
    "SunOS")
		export OS_PART=sol8-64
        ;;
    "OSF1")
		export OS_PART=tru64-gcc
        ;;
    *)
        export OS_PART=`echo $OS | sed 's/ /-/g'`
        ;;
esac;

case ${HOST_ARCH} in
    "i586")
		export ARCH=x86
        ;;
    "i686")
		export ARCH=x86
        ;;
    "x86_64")
		export ARCH=amd64
        ;;
    "sun4u")
		export ARCH=v9
        ;;
    *)
        export ARCH=`echo $HOST_ARCH | sed 's/ /-/g'`
        ;;
esac;
    
echo $ARCH-$OS_PART

