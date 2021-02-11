#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Author: Aurélien DELRIEU / <aurelien.delrieu@viveris.fr>


from distutils.core import setup

setup(
    name='opensand-network',
    version='6.0.0',
    author='Viveris Technologies',
    author_email='aurelien.delrieu@viveris.fr',
    url="http://opensand.org",
    license="GPL",
    description=('OpenSAND network'),
    scripts=[ 'opensand-network', ],
    packages=[ 'opensand_network' ],
)
