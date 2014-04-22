#!/bin/bash

RED="\\033[1;31m"
GREEN="\\033[1;32m"
NORMAL="\\033[0;39m"

mkdir -p packages/sources

function make_distcheck()
{
    dir=$1
    /bin/echo -e "**************************************"
    /bin/echo -e "Build sources for $dir"
    /bin/echo -e "**************************************"
    cd $dir
    if [ -f ./autogen.sh ]; then
        ./autogen.sh 1>/dev/null
    fi
    make distcheck 1>/dev/null  && /bin/echo -e " * ${GREEN}SUCCESS${NORMAL}" || `/bin/echo -e " * ${RED}FAILURE${NORMAL}" && exit 1`
    cd ..
    /bin/echo -e
}
function opensand()
{
    for dir in opensand-output opensand-conf opensand-rt opensand-collector opensand-core opensand-daemon opensand-manager; do
        if [ -f $dir/autogen.sh ]; then
            make_distcheck $dir
        elif [ -f $dir/setup.py ]; then
            cd $dir
            /bin/echo -e "**************************************"
            /bin/echo -e "Build sources for $dir"
            /bin/echo -e "**************************************"
            python setup.py sdist --formats=gztar 1>/dev/null && /bin/echo -e " * ${GREEN}SUCCESS${NORMAL}" || `/bin/echo -e " * ${RED}FAILURE${NORMAL}" && exit 1`
            cd ..
            /bin/echo -e
        fi
    done
}

function encap()
{
    cd opensand-plugins/encapsulation
    for dir in gse; do
        make_distcheck $dir
    done
    cd ../..
}


function lan()
{
    cd opensand-plugins/lan_adaptation
    for dir in rohc ethernet; do
        make_distcheck $dir
    done
    cd ../..
}

function att()
{
    cd opensand-plugins/physical_layer/attenuation_model
    for dir in ideal on_off triangular; do
        make_distcheck $dir
    done
    cd ../../..
}

function nom()
{
    cd opensand-plugins/physical_layer/nominal_condition/
    for dir in default; do
        make_distcheck $dir
    done
    cd ../../..
}

function min()
{
    cd opensand-plugins/physical_layer/minimal_condition/
    for dir in modcod constant; do
        make_distcheck $dir
    done
    cd ../../..
}


function err()
{
    cd opensand-plugins/physical_layer/error_insertion
    for dir in gate; do
        make_distcheck $dir
    done
    cd ../../..
}


function phy()
{
    return
#    att
#    nom
#    min
#    err
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
        lan
        ;;


    "phy")
        phy
        ;;
        
    "clean")
        rm -rf packages/sources
        ;;

    *)
        /bin/echo -e "wrong command (all, opensand, encap, lan, phy)"
        ;;
esac
