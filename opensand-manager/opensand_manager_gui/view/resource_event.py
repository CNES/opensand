#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
# Copyright © 2014 CNES
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

from opensand_manager_gui.view.resource_view import ResView
from opensand_manager_gui.view.popup.graphical_parameter import GraphicalParameter
from opensand_manager_gui.view.popup.st_assignment_dialog import AssignmentDialog

class ResEvent(ResView) :
    """ Events on configuration tab """

    def __init__(self, parent, model, manager_log):
        ResView.__init__(self, parent, model, manager_log)


    def close(self):
        """ close the configuration tab """
        self._log.debug("Resource Event: close")
        self._log.debug("Resource Event: description refresh joined")
        self._log.debug("Resource Event: closed")


#Open the forward graphical parameter 
    def on_forward_parameter_clicked(self, widget, source=None, event=None):
        window = GraphicalParameter(self._model, self._spot, self._gw,
                                    self._log, self.update_view, "forward_down")
        window.go()
#Open the return graphical parameter
    def on_return_parameter_clicked(self, widget, source=None, event=None):
        window = GraphicalParameter(self._model, self._spot, self._gw,
                                    self._log, self.update_view, "return_up")
        window.go()

#Open the forward st allocation
    def on_forward_assignment_clicked(self, widget, source=None, event=None):
        window = AssignmentDialog(self._model, self._spot, self._gw,
                                  self._log, self.update_view, 'forward_down')
        window.go()
        
#Open the return st allocation
    def on_return_assignment_clicked(self, widget, source=None, event=None):
        window = AssignmentDialog(self._model, self._spot, self._gw,
                                  self._log, self.update_view, 'return_up')
        window.go()
