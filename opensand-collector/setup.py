#!/usr/bin/env python

"""
Setup file for the OpenSAND collector.
"""

from setuptools import setup

setup(
    name="opensand-collector",
    version="trunk",
    author="Vincent Duvert",
    author_email="vduvert@toulouse.viveris.com",
    description=("Stats collector for OpenSAND platform"),
    license="GPL",
    packages=['opensand_collector'],
    scripts=["sand-collector"],
)
