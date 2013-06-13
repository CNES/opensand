#!/bin/bash

RED="\\033[1;31m"
GREEN="\\033[1;32m"
NORMAL="\\033[0;39m"

sudo /bin/echo -e ""
mkdir -p packages/all
mkdir -p packages/manager
mkdir -p packages/daemon

function build_pkg()
{
    dir=$1
    /bin/echo -e "**************************************"
    /bin/echo -e "Create package for $dir"
    /bin/echo -e "**************************************"
    cd $dir
    if [ -f ./autogen.sh ]; then
        ./autogen.sh 1>/dev/null
    fi
    dpkg-buildpackage -us -uc >/dev/null && /bin/echo -e " * ${GREEN}SUCCESS${NORMAL}" || `/bin/echo -e " * ${RED}FAILURE${NORMAL}" && exit 1`
    dh_clean 1>/dev/null
    cd ..
    /bin/echo -e

}

function opensand()
{
    for dir in opensand-conf opensand-output opensand-rt opensand-collector opensand-core opensand-daemon opensand-manager; do
        build_pkg $dir
        if [ $dir == "opensand-daemon" ]; then
            rm -rf $dir/build
        fi
        if [ $dir == "opensand-manager" ]; then
            rm -rf $dir/build
        fi
        if [ $dir == "opensand-collector" ]; then
            rm -rf $dir/build
        fi
        if [ $dir == "opensand-conf" ]; then
            sudo dpkg -i libopensand-conf*.deb 1>/dev/null
        fi
        if [ $dir == "opensand-rt" ]; then
            sudo dpkg -i libopensand-rt*.deb 1>/dev/null
        fi       
        if [ $dir == "opensand-output" ]; then
            sudo dpkg -i libopensand-output*.deb 1>/dev/null
        fi
    done
    rm *.dsc *.tar.gz *.changes
    sudo dpkg -i libopensand-plugin*.deb 1>/dev/null
}

function lan()
{
    cd opensand-plugins/lan_adaptation
    for dir in ethernet rohc ; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../..
}


function encap()
{
    cd opensand-plugins/encapsulation
    for dir in gse; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../..
}

function att()
{
    cd opensand-plugins/physical_layer/attenuation_model
    for dir in ideal on_off triangular; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}

function nom()
{
    cd opensand-plugins/physical_layer/nominal_condition/
    for dir in default; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}

function min()
{
    cd opensand-plugins/physical_layer/minimal_condition/
    for dir in modcod constant; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}


function err()
{
    cd opensand-plugins/physical_layer/error_insertion
    for dir in gate; do
        build_pkg $dir
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}

function clean()
{
    move 1>/dev/null 2>&1
    rm -rf packages
    for dir in opensand-conf opensand-output opensand-rt opensand-collector opensand-core opensand-daemon opensand-manager; do
        cd $dir
        dh_clean 1>/dev/null
        if [ $dir == "opensand-daemon" ]; then
            rm -rf build
        fi
        if [ $dir == "opensand-manager" ]; then
            rm -rf build
        fi
        cd ..
        rm *.dsc *.tar.gz *.changes  1>/dev/null 2>&1
    done
    cd opensand-plugins/lan_adaptation
    for dir in ethernet rohc ; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../..
    cd opensand-plugins/encapsulation
    for dir in gse; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../..
    cd opensand-plugins/physical_layer/attenuation_model
    for dir in ideal on_off triangular; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../../..
    cd opensand-plugins/physical_layer/nominal_condition/
    for dir in default; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../../..
    cd opensand-plugins/physical_layer/minimal_condition/
    for dir in modcod constant; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../../..
    cd opensand-plugins/physical_layer/error_insertion
    for dir in gate; do
        cd $dir
        dh_clean 1>/dev/null
        cd ..
        rm *.dsc *.tar.gz *.changes 1>/dev/null 2>&1
    done
    cd ../../..
}


function phy()
{
    att
    nom
    min
    err
}

function move()
{
	/bin/echo -e "copy packages for daemon"
	cp libopensand-conf_*.deb libopensand-plugin_*.deb  libopensand-output_*.deb libopensand-rt_*.deb opensand-core-bin_*.deb opensand-daemon_*.deb opensand-plugins/*/libopensand-*plugin_*.deb opensand-plugins/physical_layer/*/libopensand-*plugin_*.deb packages/daemon
	/bin/echo -e "copy packages for manager"
	cp libopensand-conf_*.deb libopensand-plugin_*.deb  libopensand-output_*.deb libopensand-rt_*.deb opensand-core-*_*.deb opensand-daemon_*.deb opensand-plugins/*/libopensand-*plugin_*.deb opensand-manager*.deb opensand-plugins/*/libopensand-*plugin-manager*.deb opensand-plugins/physical_layer/*/libopensand-*plugin_*.deb opensand-plugins/physical_layer/*/libopensand-*plugin-manager*.deb opensand-collector_*.deb packages/manager
	/bin/echo -e "copy packages for all"
	mv *.deb `find opensand-plugins -name \*.deb` packages/all
}

case $1 in
    "all"|"")
        opensand
        encap
        lan
        phy
        move
        ;;
    "opensand")
        opensand
        ;;

    "lan")
        lan
        ;;

    "encap")
        encap
        ;;

    "phy_att")
        att
        ;;

    "phy_nom")
        nom
        ;;

    "phy_min")
        min
        ;;

    "phy_err")
        err
        ;;

    "phy")
        phy
        ;;
        
    "clean")
        clean
        ;;
        
    "move")
        move
        ;;

    *)
        /bin/echo -e "wrong command (all, opensand, encap, lan, phy {att, nom, min, err}, clean, move)"
        ;;
esac
