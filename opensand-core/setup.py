#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Author: Aurélien DELRIEU / <aurelien.delrieu@viveris.fr>


from distutils.core import setup

setup(
    name='opensand',
    version='5.1.2',
    author='Viveris Technologies',
    author_email='aurelien.delrieu@viveris.fr',
    url="http://opensand.org",
    license="GPL",
    description=('OpenSAND Core'),
    scripts=[ 'opensand-network', ],
    packages=[ 'opensand_core' ],
)
