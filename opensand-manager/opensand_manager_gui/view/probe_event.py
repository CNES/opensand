#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2011 TAS
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
probe_event.py - the events on probe tab
"""

import gobject
import gtk
import os
import re

from opensand_manager_gui.view.probe_view import ProbeView
from opensand_manager_gui.view.popup.run_dialog import RunDialog
from opensand_manager_gui.view.popup.infos import error_popup
from opensand_manager_core.my_exceptions import ViewException

(TEXT, VISIBLE, ACTIVE) = range(3)

class ProbeEvent(ProbeView):
    """ Events for the probe tab """
    def __init__(self, parent, model, manager_log):
        ProbeView.__init__(self, parent, model, manager_log)

        self._updating = False
        self._selected_file_list = []

        self._selected = {'component' : [], 'stat' : [], 'index' : []}


    def close(self):
        """ close probe tab """
        self._log.debug("Probe Event: close")

        if self._simu_running and self._update_graph_tag is not None:
            self._stop_graph_update()
        
        self._log.debug("Probe Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        if not self._simu_running:
            return
        
        if val and self._update_graph_tag is None:
            self._start_graph_update()
        
        elif not val and self._update_graph_tag is not None:
            self._stop_graph_update()

    def on_plot_clicked(self, source=None, event=None):
        """ event handler for plot button """
        path = self._files_path
        if not self._model.is_running() and path == '':
            self._log.debug("Please import a scenario first!")
            error_popup("Please import a scenario first")
            return

        if len(self._selected['component']) == 0:
            error_popup("Please select at least a statistic")
            return

        self.init_canvas(len(self._selected['component']))

        if self._model.is_running():
            self._updating = True
            self.select_stat()
        else:
            stats = self._selected
            self._updating = False
            lgr = len(stats['component'])

            if (lgr != 0):
                self._selected_file_list = []
                i = 0
                filename = ""
                index = ""
                while(i < lgr):
                    if (stats['index'][i] == ''):
                        (filename, index, component) = \
                            self.build_file_to_plot(stats['stat'][i],
                                                    stats['component'][i], path)
                    else:
                        idx = stats['stat'][i] + "_" + stats['index'][i]
                        (filename, index, component) = \
                            self.build_file_to_plot(stats['stat'][i],
                                                    stats['component'][i],
                                                    path, idx)

                    self._selected_file_list.append((filename, index,
                                                     component))

                    i = i + 1
                self.plot_files()
                self._log.debug("Scenario files stored in '%s'" % path)
            else:
                error_popup("no statistic selected or statistic has " \
                            "substatistics")

    def build_file_to_plot(self, stat, root, path, index=""):
        """ build the selected file to plot data """
        filename = ""
        component = root[0:2]

        if component.lower() == 'st':
            inst = "%02d" % int(root[2])
            filename = path+"/stat_"+component+"_"+stat+"_"+inst+".pb"
        else:
            filename = path+"/stat_"+root.upper()+"_"+stat+".pb"

        return (filename, index, component)

    #TODO better parsing
    def read_probe_from_file(self, filename, index, component):
        """ read probes in the selected file """
        list_x = []
        list_y = []
        title = ""
        graph_type = ""

        if str(index) == '':
            self._log.debug("read stat in file '" + filename + "'")
        else:
            self._log.debug("read substat '" + index + "' in file '" +
                            filename + "'")

        try:
            probe_file = open(filename,'r')
            lines  = probe_file.readlines()
            probe_file.close()
        except IOError, error:
            error_popup("error while reading statistics for component " \
                        "%s: %s" % (component, str(error)))
            return ("", "", [], [])

        comment = "#"
        name = "<name>"
        inst = "<instance>"
        type = "<graph_type>"
        unit = "<unit>"

        # if index is empty, there is no substat, so only one column of data
        if index == '':
            idx = 1
        else:
            idx = self.get_index(filename, index)
            if idx is None:
                error_popup("index '%s' not found in file \n'%s'" %
                            (index, filename))
                return ("", "", [], [])

        # analyse the file content line by line
        line_no = 0
        for line in lines:

            line_no = line_no + 1

            # analyse the line
            if comment in line:
                # line is a comment, try to get the stat title in the comment
                if name in line:
                    name_line = line.split()
                if inst in line:
                    inst_line = line.split()
                if type in line:
                    t = line.split()
                    graph_type = t[2]
                if unit in line:
                    unit_line = line.split()

            else:
                # line contains data, check its format before appending
                # data to the plot
                line_elt = map(str.strip, line.split(';'))

                x = re.compile("\d+\.?\d*")
                m = x.match(line_elt[0])

                if len(line_elt) < 2 or m == None or \
                   float(line_elt[0]) == 0.000 or not line_elt[idx]:
                    pass
                elif line_elt[idx] == '' or line_elt[idx] == '\n':
                    error_popup("malformed line #%s in file\n'%s':\n" \
                                "[%s]" % (line_no, filename, line[:-1]))
                    break
                else:
                    list_x.append(float(line_elt[0]))
                    list_y.append(float(line_elt[idx]))


        if  component.lower() == 'st':
            if index == "":
                title = "%s%s/%s (%s)" % (component, inst_line[2],
                                          name_line[2], unit_line[2])
            else :
                title = "%s%s/%s (%s)" % (component, inst_line[2],
                                          index, unit_line[2])
        else:
            if index == "":
                title = "%s/%s (%s)" % (component, name_line[2], unit_line[2])
            else :
                title = "%s/%s (%s)" % (component, index, unit_line[2])

        return (title.replace('_',' '), graph_type, list_x, list_y)


    def get_index(self, filename, index):
        """ get in the given file the index of the column that contains
            the substat whose name is given in parameter """
        probe_file = open(filename, 'r')
        lines  = probe_file.readlines()
        idx = None

        for line in lines:
            if "time" in line:
                time = map(str.strip, line.split(';'))
                self._log.debug(str(time))

                j = 0
                while j < len(time):
                    if time[j] == index or time[j][:-1] == index:
                        self._log.debug(str(j))
                        idx = j
                        break
                    else:
                        pass
                    j = j + 1

            else:
                pass

        probe_file.close()

        return idx

    def plot_files(self):
        """ plot the content of selected files
            (should be used with gobject.idle_add outside gtk handlers) """
        #file = csv.reader(open(list[0], "rb"), delimiter=";")
        nbr = len(self._selected_file_list)

        self.init_canvas(nbr)

        i = 0
        while (i < nbr):
            filename = ""
            index = ""
            (filename, index, component) =  self._selected_file_list[i]
            (title, type, list_x, list_y) = \
                self.read_probe_from_file(filename, index, component)

            self.plot_imported_graph(i, nbr, title, type, list_x, list_y)

            i = i + 1

    def on_clear_clicked(self, source=None, event=None):
        """ event handler for clear button """
        self._probe_lock.acquire()
        self.disable_savefig_button()
        self._tree.foreach(self.clear_treeview)

        self.set_selected_stats([], [], [])

        self._updating = False
        self.init_canvas(0)
        if self._canvas is not None:
            self._canvas.hide_all()
        self._files_path = ''
        self._probe_lock.release()
        # refresh the GUI immediatly then periodically
        self.update_stat()

    def on_import_clicked(self, source=None, event=None):
        """ event handler for import button """
        scenario = ""
        run = ""

        if self._model.is_running():
            error_popup("Please stop OpenSAND to perform the importation")
        else:
            try:
                dlg = RunDialog(self._model.get_scenario(),
                                self._model.get_run())
                ret = dlg.go()
            except ViewException, msg:
                error_popup("cannot open dialog", msg)
                ret = False

            if ret:
                scenario = self._model.get_scenario()
                run = dlg.get_run()
                if run is not None:
                    self._files_path = os.path.join(scenario, run)
                    if os.path.isdir(self._files_path):
                        self._log.debug("path %s loaded" %
                                        str(self._files_path))
                    else :
                        error_popup("Run %s does not exist" % run)
                        self._files_path = ''

            dlg.close()

    def clear_treeview(self, tree, path, iterator):
        """ clear the treeview selection """
        tree.set_value(iterator, ACTIVE, False)

    def on_savefig_clicked(self, source=None, event=None):
        """ event handler for save button """
        dlg = gtk.FileChooserDialog("Save Figure", None,
                                    gtk.FILE_CHOOSER_ACTION_SAVE,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_APPLY, gtk.RESPONSE_APPLY))
        dlg.set_current_name(os.path.basename(self._files_path))
        dlg.set_current_folder(self._model.get_scenario())
        dlg.set_do_overwrite_confirmation(True)
        imgfilter = gtk.FileFilter()
        imgfilter.add_pixbuf_formats()
        imgfilter.set_name('All images')
        dlg.add_filter(imgfilter)
        allfilter = gtk.FileFilter()
        allfilter.add_pattern("*")
        allfilter.set_name('All files')
        dlg.add_filter(allfilter)
        ret = dlg.run()
        filename = dlg.get_filename()
        if ret == gtk.RESPONSE_APPLY and filename is not None:
            (name, ext) = os.path.splitext(filename)
            if ext == '':
                # set default filetype to svg, jpg gives bad results
                filename = filename + '.svg'
            self._log.debug("save graphic to " + filename)
            self.save_figure(filename)

        dlg.destroy()
