#!/bin/sh

set -x

PARAMSTR=$*
SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install}
BUILD_NO_EXAMPLES=${BUILD_NO_EXAMPLES:-1}

mkdir -p $BUILD_DIR/$BUILD_TYPE \
  && mkdir -p $BUILD_DIR/$BUILD_TYPE/bin \
  && cd $BUILD_DIR/$BUILD_TYPE \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_BUILD_NO_EXAMPLES=$BUILD_NO_EXAMPLES \
           $SOURCE_DIR \
  && make $*

#[[ $# = 0 ]] && cp $SOURCE_DIR/conf/* bin

#[[ $PARAMSTR =~ "install" ]] \
#  && mkdir -p $INSTALL_DIR/rdngatesvr $INSTALL_DIR/rdnurisvr $INSTALL_DIR/rdnsyncli $INSTALL_DIR/rsc \
#  && cp $SOURCE_DIR/conf/rdngatesvr.ini $SOURCE_DIR/conf/rdngatesvr.properties $INSTALL_DIR/rdngatesvr/ \
#  && cp $INSTALL_DIR/bin/rdngatesvr $INSTALL_DIR/rdngatesvr/ \
#  && cp $SOURCE_DIR/conf/rdnurisvr.ini $SOURCE_DIR/conf/rdnurisvr.properties $INSTALL_DIR/rdnurisvr/ \
#  && cp $INSTALL_DIR/bin/rdnurisvr $INSTALL_DIR/rdnurisvr/ \
#  && cp $SOURCE_DIR/conf/rdnsyncli.ini $SOURCE_DIR/conf/rdnsyncli.properties $INSTALL_DIR/rdnsyncli/ \
#  && cp $INSTALL_DIR/bin/rdnsyncli $INSTALL_DIR/rdnsyncli/ \
#  && cp $SOURCE_DIR/conf/rsc.ini $SOURCE_DIR/conf/rsc.properties $SOURCE_DIR/conf/rsc_update_note.txt $INSTALL_DIR/rsc/ \
#  && cp $SOURCE_DIR/conf/rdn_update_note.txt $INSTALL_DIR/ \
#  && cp $INSTALL_DIR/bin/rsc $INSTALL_DIR/rsc/ \
#  && rm -rf $INSTALL_DIR/bin $INSTALL_DIR/lib $INSTALL_DIR/include


# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# cd $SOURCE_DIR && doxygen

 