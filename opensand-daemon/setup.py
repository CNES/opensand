#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import os
from setuptools import setup, find_packages
from distutils.core import Extension

# Utility function to read the README file.
# Used for the long_description.  It's nice, because now 1) we have a top level
# README file and 2) it's easier to type in the README file than to put a raw
# string in below ...
def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

# List of files
script_files = ['scripts/host_init',
                'scripts/write_initialize_config',
                'scripts/module_support.py', 'scripts/tool_support.py']
bin_files = ['sand-daemon']

# libnl files
opts = ['-O', '-nodefaultctor']
include = ['/usr/include/libnl3']

netlink_capi = Extension('netlink/_capi',
                         sources = ['netlink/capi.i'],
             include_dirs = include,
             swig_opts = opts,
             libraries = ['nl-3'],
            )

route_capi = Extension('netlink/route/_capi',
                         sources = ['netlink/route/capi.i'],
             include_dirs = include,
             swig_opts = opts,
             libraries = ['nl-3', 'nl-route-3'],
            )

setup(
    name="opensand-daemon",
    version="trunk",
    author="Julien Bernard",
    author_email="jbernard@toulouse.viveris.com",
    description=("Daemon for OpenSAND entity (sat, gw, st or ws)"),
    license="GPL",
    url="http://opensand.org",
    ext_modules=[netlink_capi, route_capi],

    packages=find_packages(),

    data_files=[('libexec/opensand/', script_files)],
    scripts=bin_files
)

