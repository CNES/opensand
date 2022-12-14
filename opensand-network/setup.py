#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Author: Aurélien DELRIEU / <aurelien.delrieu@viveris.fr>

import re
from setuptools import setup


def version(filename):
    with open(filename) as f:
        for line in f:
            if match := re.search(r'opensand-(\d+\.\d+\.\d+)', line):
                return match.group(1)
    raise RuntimeError(f'No version found in file {filename}')


setup(
    name='opensand-network',
    version=version('ChangeLog'),
    author='Viveris Technologies',
    author_email='aurelien.delrieu@viveris.fr',
    url="http://opensand.org",
    license="GPL",
    description=('OpenSAND network'),
    scripts=['opensand-network'],
    packages=['opensand_network'],
)
