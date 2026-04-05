#!/bin/bash
# Script outline to install and build kernel.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "${OUTDIR}"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Building the kernel"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE -j4 Image dtbs
    cp "${OUTDIR}/linux-stable/arch/arm64/boot/Image" "${OUTDIR}/Image"
fi

echo "Copying kernel Image to OUTDIR"

echo "Creating the staging directory for the root filesystem"
cd "${OUTDIR}"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Creating rootfs structure"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

mkdir -p bin \
    dev \
    etc \
    home \
    lib \
    lib64 \
    proc \
    sbin \
    sys \
    tmp \
    usr \
    usr/bin \
    usr/lib \
    usr/sbin \
    var \
    var/log

cd "${OUTDIR}"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone https://github.com/mirror/busybox
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    echo "Configuring busybox"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE distclean
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
else
    cd busybox
fi

echo "Building busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j4
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

# Ensure sh exists for init
cd "${OUTDIR}/rootfs/bin"
ln -sf busybox sh

echo "Library dependencies"
${CROSS_COMPILE}readelf -a busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a busybox | grep "Shared library"

cd ${OUTDIR}/rootfs

echo "Adding library dependencies"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
ROOTFS="${OUTDIR}/rootfs"
TARGET="${ROOTFS}/bin/busybox"

INTERPRETER=$(${CROSS_COMPILE}readelf -l ${TARGET} | grep "program interpreter" | awk -F ': ' '{print $2}' | tr -d ']')
INTERPRETER_LIB=$(basename "${INTERPRETER}")
find "${SYSROOT}" -name "${INTERPRETER_LIB}" -exec cp -L {} "${ROOTFS}/lib64/" \;

SLIBS=$(${CROSS_COMPILE}readelf -d "${TARGET}" | grep "Shared library" | awk -F '[' '{print $2}' | tr -d ']')
for LIB in ${SLIBS}; do
    find "${SYSROOT}" -name "${LIB}" -exec cp -L {} "${ROOTFS}/lib64/" \;
done

# Ensure ELF loader exists at the exact path expected by the kernel
cp -L "${SYSROOT}/lib/ld-linux-aarch64.so.1" "${ROOTFS}/lib/"

${CROSS_COMPILE}strip --strip-unneeded "${ROOTFS}/lib64"/* 2>/dev/null || true 

cd "${ROOTFS}"
ln -sfn lib64 lib
echo "Creating device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

echo "Building the writer utility"
cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

echo "Copying writer and finder scripts to home..."
cp writer "${ROOTFS}/home/"
mkdir -p "${ROOTFS}/home/conf/"
cp finder.sh "${ROOTFS}/home/"
cp finder-test.sh "${ROOTFS}/home/"
cp ../conf/* "${ROOTFS}/home/conf/"
cp autorun-qemu.sh "${ROOTFS}/home/"


# Chown the root directory
echo "Creating initramfs..."
sudo chown -R root:root "${ROOTFS}"

# Create initramfs.cpio.gz
cd "${ROOTFS}"
find . | cpio -ov -H newc --owner root:root > "${OUTDIR}/initramfs.cpio"
cd "${OUTDIR}"
gzip -f initramfs.cpio
