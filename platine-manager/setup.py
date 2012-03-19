# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

import os
from setuptools import setup, find_packages

# Utility function to read the README file.
# Used for the long_description.  It's nice, because now 1) we have a top level
# README file and 2) it's easier to type in the README file than to put a raw
# string in below ...
def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

# List of files
img_files = ['images/' + png for png in os.listdir('images') if png.endswith('.png')]
data_files = ['platine.glade']
script_files = ['searchEntry.sh']

#packages = find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"])

setup(
    name="PtManager",
    version="0.1",
    author="Julien Bernard",
    author_email="jbernard@toulouse.viveris.com",
    description=("Manager for Platine emulation testbed"),
    license="GPL",
#    long_description=read('README'),

    # Find Platine Manager packages, but exclude tests
    packages=find_packages(),

    data_files=[
                  ('bin/', ['PtManager']), # binary
                  # PtManager files
                  ('share/platine/manager', data_files),
                  ('share/platine/manager/images', img_files),
                  ('libexec/platine/', script_files),
                  ]
)
