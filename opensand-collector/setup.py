#!/usr/bin/env python2
# -*- coding: utf-8 -*-

"""
Setup file for the OpenSAND collector.
"""

from setuptools import setup

setup(
    name="opensand-collector",
    version="trunk",
    author="Vincent Duvert",
    author_email="vduvert@toulouse.viveris.com",
    description=("Statistics and events collector for OpenSAND platform"),
    license="GPL",
    packages=['opensand_collector'],
    scripts=["sand-collector"],
)
