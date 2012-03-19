import os
from setuptools import setup, find_packages

# Utility function to read the README file.
# Used for the long_description.  It's nice, because now 1) we have a top level
# README file and 2) it's easier to type in the README file than to put a raw
# string in below ...
def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

# List of files
script_files = ['scripts/sat_netinit', 'scripts/st_netinit', 'scripts/write_initialize_config', 'scripts/remove_config']
bin_files = ['PtDmon']

#packages = find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"])

setup(
    name="PtDmon",
    version="3.0.0",
    author="Julien Bernard",
    author_email="jbernard@toulouse.viveris.com",
    description=("Daemon for Platine entity (sat, gw, st or ws)"),
    license="GPL",
#    long_description=read('README'),

    packages=find_packages(),

    data_files=[
                  ('bin/', bin_files), # binary
                  # PtDaemon files
                  ('libexec/platine/', script_files),
                  ]
)
