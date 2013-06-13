#!/bin/bash

mkdir -p packages/sources
function opensand()
{
    for dir in opensand-conf opensand-rt opensand-output opensand-collector opensand-core opensand-daemon opensand-manager; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
            make distcheck 1>/dev/null
        elif [ -f ./setup.py ]; then
            python setup.py sdist --formats=gztar 1>/dev/null
        fi
        cd ..
        echo
    done
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
        make distcheck 1>/dev/null
        cd ..
        echo
    done
    cd ../..
}


function lan()
{
    cd opensand-plugins/lan_adaptation
    for dir in rohc ethernet; do
        echo "**************************************"
        echo "Create package for $dir"
        echo "**************************************"
        cd $dir
        if [ -f ./autogen.sh ]; then
            ./autogen.sh 1>/dev/null
        fi
        make distcheck 1>/dev/null
        cd ..
        echo
    done
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
        make distcheck 1>/dev/null
        cd ..
        echo
    done
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
        make distcheck 1>/dev/null
        cd ..
        echo
    done
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
        make distcheck 1>/dev/null
        cd ..
        echo
    done
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
        make distcheck 1>/dev/null
        cd ..
        echo
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

move()
{
    mv `find . -name \*.tar.gz` packages/sources
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

    "encap")
        encap
        ;;
        
    "lan")
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
        rm -rf packages/sources
        ;;

    *)
        echo "wrong command (all, opensand, encap, lan, phy {att, nom, min, err})"
        ;;
esac
