#!/bin/bash

sudo echo ""
mkdir -p packages/all
mkdir -p packages/manager
mkdir -p packages/daemon
function opensand()
{
    for dir in opensand-conf opensand-output opensand-rt opensand-collector opensand-core opensand-daemon opensand-manager; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        if [ $dir == "opensand-daemon" ]; then
            rm -rf build
        fi
        if [ $dir == "opensand-manager" ]; then
            rm -rf build
        fi
        cd ..
        if [ $dir == "opensand-conf" ]; then
            sudo dpkg -i libopensand-conf*.deb
        fi
        if [ $dir == "opensand-rt" ]; then
            sudo dpkg -i libopensand-rt*.deb
        fi       
        if [ $dir == "opensand-output" ]; then
            sudo dpkg -i libopensand-output*.deb
        fi
        echo
    done
    rm *.dsc *.tar.gz *.changes
    sudo dpkg -i libopensand-plugin*.deb
}

function lan()
{
    cd opensand-plugins/lan_adaptation
    for dir in ethernet rohc ; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../..
}


function encap()
{
    cd opensand-plugins/encapsulation
    for dir in gse; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../..
}

function att()
{
    cd opensand-plugins/physical_layer/attenuation_model
    for dir in ideal on_off triangular; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}

function nom()
{
    cd opensand-plugins/physical_layer/nominal_condition/
    for dir in default; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}

function min()
{
    cd opensand-plugins/physical_layer/minimal_condition/
    for dir in modcod constant; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}


function err()
{
    cd opensand-plugins/physical_layer/error_insertion
    for dir in gate; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        dpkg-buildpackage 1>/dev/null
        dh_clean 1>/dev/null
        cd ..
        echo
    done
    rm *.dsc *.tar.gz *.changes
    cd ../../..
}


function phy()
{
    att
    nom
    min
    err
}

move()
{
	echo "copy packages for daemon"
	cp libopensand-conf_*.deb libopensand-plugin_*.deb  libopensand-output_*.deb libopensand-rt_*.deb opensand-core-bin_*.deb opensand-daemon_*.deb opensand-plugins/*/libopensand-*plugin_*.deb opensand-plugins/physical_layer/*/libopensand-*plugin_*.deb packages/daemon
	echo "copy packages for manager"
	cp libopensand-conf_*.deb libopensand-plugin_*.deb  libopensand-output_*.deb libopensand-rt_*.deb opensand-core-*_*.deb opensand-daemon_*.deb opensand-plugins/*/libopensand-*plugin_*.deb opensand-manager*.deb opensand-plugins/*/libopensand-*plugin-manager*.deb opensand-plugins/physical_layer/*/libopensand-*plugin_*.deb opensand-plugins/physical_layer/*/libopensand-*plugin-manager*.deb opensand-collector_*.deb packages/manager
	echo "copy packages for all"
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
        rm -rf packages/*/*.deb
        ;;
        
    "move")
        move
        ;;

    *)
        echo "wrong command (all, opensand, encap, lan, phy {att, nom, min, err}, clean, move)"
        ;;
esac
