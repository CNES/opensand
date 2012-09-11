# -*- coding: utf8 -*-

"""
This module is used to start the collector without installing it into the
system.

This can be done with the command "python -m opensand_collector" (or
"python -m opensand_collector.__main__" for Python < 2.7)
"""

from opensand_collector import OpenSandCollector

if __name__ == '__main__':
    OpenSandCollector().run()
