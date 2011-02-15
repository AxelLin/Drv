#!/bin/sh

COLOR_RED="[1;31m"
COLOR_NORMAL="[0;39m"
COLOR_GREEN="[1;32m"

SDK_RELEASE=Y

PWD_DIR=$PWD
PCIV_DIR=$PWD/../

if [ "$SDK_RELEASE" = "Y" ]; then
    PCIDRV_DIR=$PWD/../../../../source/drv/hisi-pci/
    IMAGE_DIR=$PWD/../../../../pub/
    MPPKO_DIR=$PWD/../../../ko.rel/
    TDEKO_DIR=$MPPKO_DIR
    HIFBKO_DIR=$MPPKO_DIR
fi

if [ "$SDK_RELEASE" = "N" ]; then
    PCIDRV_DIR=/hiwork/hi3511sdk/osdrv/source/drv/hisi-pci/
    IMAGE_DIR=/hiwork/hi3511sdk/osdrv/pub
    MPPKO_DIR=$PWD/../../../code/
    TDEKO_DIR=$PWD/../../../../cbb/tde/
    HIFBKO_DIR=$PWD/../../../../cbb/hifb/
fi

report_error(){
    echo "${COLOR_RED}******* Error!! Shell exit for error *****${COLOR_NORMAL}"
    exit
}

build_mpp(){
    pushd $MPPKO_DIR || report_error
    
    if [ "$SDK_RELEASE" = "N" ]; then
        make || report_error
    fi
    
    rm $PWD_DIR/meadia_driver/*3511* -fr  || report_error  
    cp hi3511*.ko $PWD_DIR/meadia_driver/ -f  || report_error
    ex +"%s/\(_MOD_DIR=\).*/\1./" +"w $PWD_DIR/meadia_driver/load3511" +"q" load3511
    chmod +x $PWD_DIR/meadia_driver/load3511
    
    popd
    echo "${COLOR_GREEN}******* Build MPP success! Copy all the ko to ./meadia_driver *****${COLOR_NORMAL}"
}

build_tde(){
    pushd $TDEKO_DIR  || report_error
    
    if [ "$SDK_RELEASE" = "N" ]; then
        make || report_error
    fi
        
    cp tde.ko $PWD_DIR/meadia_driver/ -f
    
    popd
    echo "${COLOR_GREEN}******* Build TDE success! Copy all the ko to ./meadia_driver *****${COLOR_NORMAL}"
}

build_hifb(){
    pushd $HIFBKO_DIR || report_error
    
    if [ "$SDK_RELEASE" = "N" ]; then
        make || report_error
    fi
        
    cp hifb.ko $PWD_DIR/meadia_driver/ -f
    
    popd
    echo "${COLOR_GREEN}******* Build HIFB success! Copy all the ko to ./meadia_driver *****${COLOR_NORMAL}"
}

build_pciv(){
    pushd $PCIV_DIR || report_error
    make || report_error
    cp *.ko pciv_test_host pciv_test_slave $PWD_DIR/test/ -f || report_error
    popd
    echo "${COLOR_GREEN}******* Build PCIV success! Copy all the ko to ./test *****${COLOR_NORMAL}"
}

build_pcidrv(){
    pushd $PCIDRV_DIR || report_error
    make clean || report_error
    make HI_PCI_TARGET=slave || report_error
    make || report_error

    cp pci_multi_boot.ko $PWD_DIR/boot/ -f
    cp hi3511_pci_trans.ko $PWD_DIR/pci_driver/ -f
    cp pci_hwhal_host.ko pci_hwhal_slave.ko  $PWD_DIR/pci_driver/ -f
    cp ./hi_pcicom_v2/hios_mcc/mcc_usrdev.ko $PWD_DIR/pci_driver/ -f
    cp ./hi_pcicom_v2/hios_pci/hi_hw_adp.ko  $PWD_DIR/pci_driver/ -f
    cp ./hi_pcicom_v2/hios_pci/pcicom.ko     $PWD_DIR/pci_driver/ -f
    
    popd
    echo "${COLOR_GREEN}******* Build PCI Driver success! Copy all the ko to ./pci_driver *****${COLOR_NORMAL}"
}

build_clr(){
    rm  $PWD_DIR/meadia_driver/* -fr
    rm  $PWD_DIR/pci_driver/* -fr
    rm  $PWD_DIR/test/*.ko -fr
    rm  $PWD_DIR/test/*test* -fr
    rm  $PWD_DIR/boot/*.ko -fr
    rm  $PWD_DIR/boot/rootfs-FULL_REL/root/* -fr
}

mk_slave_image(){
    pushd $PWD_DIR/boot || report_error
    if [ ! -d rootfs-FULL_REL ]; then
        cp $IMAGE_DIR/tarball/rootfs-FULL_REL-Flash.tgz ./  || report_error
        cp $IMAGE_DIR/images/kernel-hi3511v100dmeb_full_release.img ./kernel.img  || report_error
        
        tar -xzf rootfs-FULL_REL-Flash.tgz
        cd rootfs-FULL_REL; sudo ./mknod_console; cd ..
    fi

    cp $PWD_DIR/meadia_driver ./rootfs-FULL_REL/root/ -af || report_error
    cp $PWD_DIR/pci_driver    ./rootfs-FULL_REL/root/ -af || report_error
    cp $PWD_DIR/test          ./rootfs-FULL_REL/root/ -af || report_error
    cp $PWD_DIR/slave.sh      ./rootfs-FULL_REL/root/ -f  || report_error
    
    ./mkimg.rootfs || report_error
    
    popd
    echo "${COLOR_GREEN}******* Build Slave Image success! *****${COLOR_NORMAL}"
}

if [ $# -eq 0 ];then
    echo "Usage as $0 { all }|{ [mpp] [tde] [hifb] [pciv] [pcidrv] [clr] }"
    exit 0
fi

if [ "$1" = "all" ]; then
    build_clr
    build_mpp
    build_tde
    build_hifb
    build_pciv
    build_pcidrv
else

    while [ $# -ne 0 ]
    do
        build_$1
        shift
    done
fi

#copy the bin file to slave 
mk_slave_image
