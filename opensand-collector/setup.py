#!/usr/bin/env python2
# -*- coding: utf-8 -*-

"""
Setup file for the OpenSAND collector.
"""

from setuptools import setup, find_packages

setup(
    name="opensand-collector",
    version="4.0.0",
    author="Vincent Duvert",
    author_email="vduvert@toulouse.viveris.com",
    description=("Statistics and events collector for OpenSAND platform"),
    license="GPL",
    url="http://opensand.org",
    packages=find_packages(),
    scripts=["sand-collector"],
)
