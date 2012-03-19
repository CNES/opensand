#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# Platine is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
#
#
# This file is part of the Platine testbed.
#
#
# Platine is free software : you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
probe.py - probe element
"""

class Probe:
    """ Probe object """
    def __init__(self, id, name):
        self._stat_list = []
        self._name = name
        self._id = id

    def add_stat(self, stat):
        """ add a stat in the stat list """
        self._stat_list.append(stat)

    def get_stat_list(self):
        """ get the stat list """
        return self._stat_list

    def get_name(self):
        """ get probe name """
        return self._name

class Index:
    """ index of plots """
    def __init__(self, idx_id, idx_name, stat):
        self._id = idx_id
        self._name = idx_name
        self._ymin = -10
        self._ymax = 10
        self._xmin = 0
        self._xmax = 0

        self._time = []
        self._value = []
        self._stat = stat
        self._state = False
        self._curtime = None
        self._curvalue = None
        self._oldtime = None
        self._oldvalue = None

    def set(self, time, value):
        """ set a new index value """
        self._oldtime = self._curtime
        self._oldvalue = self._curvalue

        self._curtime = time
        self._curvalue = value

    def add(self, time, value):
        """ add an index value """
        if len(self._time) > 30:
            self._time.remove(self._time[0])
            self._value.remove(self._value[0])

        self._time.append(time)
        self._value.append(value)

        self._ymin = min(self._value)
        self._ymax = max(self._value)
        self._xmin = min(self._time)
        self._xmax = max(self._time)

    def get_xmin(self):
        """ get the xmin value """
        return self._xmin

    def get_xmax(self):
        """ get the xmax value """
        return self._xmax

    def get_ymin(self):
        """ get the xmin value """
        return self._ymin

    def get_ymax(self):
        """ get the ymax value """
        return self._ymax

    def get_name(self):
        """ get the index name """
        return self._name

    def get_stat(self):
        """ get the index stat """
        return self._stat

    def get_value(self):
        """ get the values list """
        return self._value

    def is_value(self):
        """ check if there is at least one value """
        if len(self._value) > 0:
            return True
        return False

    def get_x_y(self):
        """ get the index coordinates """
        if len(self._value) > 0:
            return (self._time, self._value)
        else:
            return None

    def get_id(self):
        """ get the index id """
        return self._id

    def get_time(self):
        """ get the index time """
        return self._time

class Stat:
    """ Statistics object """
    def __init__(self, id, name, cat, unit_type, unit, graph_type, comment):
        self._id = id
        self._name = name.lstrip()
        self._category_id = cat.lstrip()
        self._type = unit_type.lstrip()
        self._unit = unit.lstrip()
        self._graph_type = graph_type.lstrip()
        self._comment = comment.lstrip()
        self._index_list = []

    def get_index_list(self):
        """ get the stat index list """
        return self._index_list

    def get_name(self):
        """ get the stat name """
        return self._name

    def get_unit(self):
        """ get the stat unit """
        return self._unit

    def get_comment(self):
        """ get the stat comment """
        return self._comment

    def get_graph_type(self):
        """ get the stat graphic type """
        return self._graph_type

    def debug(self):
        """ print statistics information """
        print 'stat name ' + self._name
        print '\tcategory_id ' + self._category_id
        print '\ttype ' + self._type
        print '\tunit ' + self._unit
        print '\tgraph_type ' + self._graph_type
        print '\tcomment ' + self._comment
