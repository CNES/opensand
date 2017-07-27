#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2017 TAS
# Copyright © 2017 CNES
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistribute it and/or modify it under the
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
resource_event.py - the events on resources configuration tab
"""

from opensand_manager_gui.view.resource_view import ResourceView
from opensand_manager_gui.view.popup.graphical_parameter import GraphicalParameter
from opensand_manager_gui.view.popup.st_assignment_dialog import AssignmentDialog
from opensand_manager_core.utils import FORWARD_DOWN, RETURN_UP

class ResourceEvent(ResourceView):
    """ Events on configuration tab """

    def __init__(self, parent, model, manager_log):
        ResourceView.__init__(self, parent, model, manager_log)


    def close(self):
        """ close the configuration tab """
        self._log.debug("Resource Event: close")
        self._log.debug("Resource Event: description refresh joined")
        self._log.debug("Resource Event: closed")


    def on_forward_parameter_clicked(self, widget, source=None, event=None):
        """ open the forward graphical parameters """
        window = GraphicalParameter(self._model, self._spot, self._gw,
                                    self._fmt_group[FORWARD_DOWN],
                                    self._forward_carrier_arithmetic,
                                    self._log, self.update_view, FORWARD_DOWN)
        window.go()


    def on_return_parameter_clicked(self, widget, source=None, event=None):
        """ open the return graphical parameters """
        window = GraphicalParameter(self._model, self._spot, self._gw,
                                    self._fmt_group[RETURN_UP],
                                    self._return_carrier_arithmetic,
                                    self._log, self.update_view, RETURN_UP)
        window.go()


    def on_forward_assignment_clicked(self, widget, source=None, event=None):
        """ open the forward ST allocation """
        window = AssignmentDialog(self._model, self._spot, self._gw,
                                  self._log, self.update_view, FORWARD_DOWN)
        window.go()
        

    def on_return_assignment_clicked(self, widget, source=None, event=None):
        """ open the return ST allocation """
        window = AssignmentDialog(self._model, self._spot, self._gw,
                                  self._log, self.update_view, RETURN_UP)
        window.go()
