#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
probe_model.py - model for probes
"""

import os

from platine_manager_core.my_exceptions import ProbeException
from platine_manager_gui.view.probes.probe import Probe, Stat, Index

PROBE_CONF_PATH = '/etc/platine/env_plane/'

class ProbeModel:
    """ list of probes """
    def __init__(self, model, lock, manager_log):
        self._model = model
        self._log = manager_log

        self._probe_lock = lock

        self._list = []
        self._next_probe_index = 0
        self._next_stat_index = 0
        self._next_substat_index = 0

    def create(self, host_names):
        """ initialize the statistics list from the configuration file """
        stat_conf_path = PROBE_CONF_PATH
        if not os.path.isdir(stat_conf_path):
            raise ProbeException("probe configuration path '%s' does not exist"
                                 % stat_conf_path)

        component = ''
        for name in host_names:
            component = name
            if name.startswith('st'):
                component = 'st'
            conf_file = "%sstat_def_%s.conf" % (stat_conf_path,
                                                component)
            try:
                self._log.debug("add probes for %s from file '%s'" %
                                (name, conf_file))
                self.add_probe(name, conf_file)
            except IOError, (errno, strerror):
                raise ProbeException("failed to add probes for %s from file " \
                                     "'%s': %s" % (name, conf_file, strerror))

    def add_probe(self, name, conf_file):
        """ add a probe in the probe list """
        # open the probe conf file
        try:
            conf = open(conf_file)
        except IOError:
            raise

        # start read the file
        line = conf.readline()

        # get an unique ID for the new probe
        probe_id = self._next_probe_index
        self._next_probe_index = self._next_probe_index + 1

        probe = Probe(probe_id, name)

        # TODO more error handling
        #      (such as bad file format :-) and better parsing)
        while line != '':
            line = line.strip()
            if not line.startswith('/*') and not line.startswith('#'):
                # this is not comment, continue
                if line.startswith('Statistic_number'):
                    args = line.split(':')
                    nb_stat = int(args[1])
                    # skip the {
                    conf.readline()
                    for stat in range(nb_stat):
                        # read the stat definition
                        line = conf.readline()
                        args = line.strip().split(',')

                        # create the stat instance
                        stat = self.add_stat(probe, args[0], args[1], \
                                             args[2], args[3], args[4], \
                                             args[5])

                        #stat.debug()

                        nb_index = int(args[6])
                        # statistics with different indexes
                        if nb_index != 0:
                            # skip the {
                            conf.readline()
                            for index in range(nb_index):
                                line = conf.readline().strip()
                                self.add_index(line, stat)
                            # skip the }
                            conf.readline()
                        else:
                            # statistic without index
                            self.add_index('', stat)

            line = conf.readline()

        conf.close()

        self._list.append(probe)

    def remove_probe(self, probe):
        """ remove a probe from the probe list """
        self._log.debug("remove probe for " + probe.get_name())
        self._list.remove(probe)

    def add_stat(self, probe, name, cat, unit_type, unit, graph_type, comment):
        """ create a stat to add into list """
        # get an unique ID for the new stat
        stat_id = self._next_stat_index
        self._next_stat_index = self._next_stat_index + 1
        # create the stat object
        stat = Stat(stat_id, name, cat, unit_type, unit, graph_type, comment)
        probe.add_stat(stat)
        return stat

    def add_index(self, idx_name, stat):
        """ create an index for stat """
        # get an unique ID for the new substat
        idx_id = self._next_substat_index
        self._next_substat_index = self._next_substat_index + 1
        # create the substat object
        index = Index(idx_id, idx_name, stat)
        stat.get_index_list().append(index)

    def find_stat(self, component, stat_name, idx):
        """ find a substat object given a component name,
            a stat_name and a substat index """
        if str(idx) == '':
            self._log.debug("find object for component '%s' and stat '%s'" \
                            % (component, stat_name))
        else:
            self._log.debug("find the substat with index %s for component " \
                            "'%s' and stat '%s'" % (idx, component, stat_name))

        self._probe_lock.acquire()
        for probe in self._list:
            if probe.get_name() == component.lower():
                for stat in probe.get_stat_list():
                    if stat.get_name() == stat_name:
                        for index in stat.get_index_list():
                            if (index.get_name() == '') or \
                               (index.get_name() == idx):
                                self._log.debug("index found %s %s datas" %
                                                (index.get_name(),
                                                len(index.get_time())))
                                self._probe_lock.release()
                                return index
        self._probe_lock.release()
        return None

    def add_value(self, component, stat_id, idx_id, value, horodate):
        """ add a stat value """
        self._probe_lock.acquire()
        if idx_id != 0:
            idx_id = idx_id - 1
        if stat_id != 0:
            stat_id = stat_id - 1
        for probe in self._list:
            if probe.get_name() == component.lower():

                # the statistic id is from 1 to n
                stat_list = probe.get_stat_list()
                if stat_id > len(stat_list) - 1 or stat_id < 0:
                    self._log.debug("%s stat_id (%s) outside authorized " \
                                    "values [0->%s]" % (component, stat_id,
                                    len(stat_list)))
                    self._probe_lock.release()
                    return
                stat = stat_list[stat_id]
                if idx_id > len(stat.get_index_list())-1 or idx_id < 0:
                    self._log.debug("%s %s idx_id (%s) outside authorized " \
                                    "values [0->%s]" % (component,
                                    stat.get_name(), idx_id,
                                    len(stat.get_index_list())))
                    self._probe_lock.release()
                    return

                index = stat.get_index_list()[idx_id]

                # add the value
                index.set(horodate, value)
                index.add(horodate, value)
        self._probe_lock.release()

    def get_list(self):
        """ get the probe list """
        return self._list
