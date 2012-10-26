# -*- coding: utf-8 -*-

"""
probe_display.py - handles the display of probe values on graphs.
"""

from collections import deque
from matplotlib.backends.backend_gtkagg import FigureCanvasGTK
from matplotlib.ticker import FormatStrFormatter
import gobject
import matplotlib.pyplot as plt
import random

GRAPH_MAX_POINTS = 30
TIME_FORMATTER = FormatStrFormatter('%.1f')
VALUE_FORMATTER = FormatStrFormatter('%2.8g')

class ProbeGraph(object):
    """
    Represents the graph for a probe
    """

    def __init__(self, display, program_name, probe_name, unit):
        self._display = display
        probe_name = probe_name.replace("_", " ").replace(".", ": ")
        self._title = "[%s] %s" % (program_name, probe_name)
        
        if unit:
            self._title += " (%s)" % unit
        
        self.index = 0

        self._times = deque(maxlen=GRAPH_MAX_POINTS)
        self._values = deque(maxlen=GRAPH_MAX_POINTS)

        self._axes = None
        self._xaxis = None
        self._yaxis = None
        self._color = random.choice("bgrcmyk")
        self._dirty = False
    
    def setup(self, axes, direct_values=None):
        """
        Sets up the graph display.
        """
        
        self._axes = axes
        self._xaxis = axes.get_xaxis()
        self._yaxis = axes.get_yaxis()

        if direct_values is not None:
            times, self._values = direct_values
            self._times = [time / 1000.0 for time in times]

        self._redraw()
        
    def update(self):
        """
        Updates the graph display to take new values into account
        """
    
        if not self._dirty:
            return False
    
        self._redraw()
        
        self._dirty = False
        
        return True

    def add_value(self, time, value):
        """
        Appends a value to the graph
        """
        
        self._times.append(time / 1000.0)
        self._values.append(value)
        
        self._dirty = True
    
    def _redraw(self):
        """
        Plots the current values
        """
        
        self._axes.cla()
        x = self._times
        y = self._values
        
        try:
            xmin, xmax = min(x), max(x)
            ymin, ymax = min(y), max(y)
        except ValueError:
            xmin, xmax, ymin, ymax = (0, 0, 0, 0)
        
        if xmin == xmax:
            xmax = xmin + 1
        
        if ymin == ymax:
            ymax = ymin + 1
        
        rymin = ymin - ((ymax - ymin) * 0.1)
        rymax = ymax + ((ymax - ymin) * 0.1)
        
        self._axes.set_title(self._title, size="small")
        self._xaxis.set_major_formatter(TIME_FORMATTER)
        self._yaxis.set_major_formatter(VALUE_FORMATTER)

        self._axes.plot(x, y, '-', color=self._color)
        
        self._axes.axis([xmin, xmax, rymin, rymax])
    

class ProbeDisplay(object):
    """
    Handler for the graph display.
    """
    
    def __init__(self, parent_box):
        self._displayed_probes = {}
        self.num_graphs = 0
        
        self._fig = plt.figure()
        self._fig.subplots_adjust(hspace=0.8)
        self._canvas = FigureCanvasGTK(self._fig)
        parent_box.pack_start(self._canvas, True, True)
        self._canvas.show()
        self._probe_data = {}
    
    def save_figure(self, filename):
        """
        Saves the figure to a file.
        """
    
        self._fig.savefig(filename)
    
    def set_probe_data(self, probe_data=None):
        """
        Called to provide direct probe data to the display.
        """
        
        if probe_data is None:
            self._probe_data = {}
        else:
            self._probe_data = probe_data
        
    def update(self, displayed_probes):
        """
        Called when the probe display need to be updated (e.g. the list of
        displayed probes has changed)
        """
    
        gobject.idle_add(self._update, displayed_probes)
    
    def _update(self, displayed_probes):
        """
        Updates the probe display.
        """
        
        new_probes = []
        max_index = len(self._displayed_probes) + 1
                
        for probe in displayed_probes:
            graph = self._displayed_probes.get(probe.global_ident)
            
            probe_data = self._probe_data.get(probe.full_name)
            
            if not graph:
                graph = ProbeGraph(self, probe.program.name, probe.name,
                    probe.unit)
                graph.index = max_index # Temporary, to put it at the end
            
            new_probes.append((probe.global_ident, graph, probe_data))
        
        new_probes.sort(key=lambda item: item[1].index)
        
        self._displayed_probes = {}
        self._fig.clear()
        
        # Now update the graphs, and construct the new _displayed_probes dict
        self.num_graphs = len(new_probes)
        
        if self.num_graphs == 0:
            self._canvas.draw_idle()
            return
        
        for i, (probe_ident, graph, probe_data) in enumerate(new_probes):
            axes = self._fig.add_subplot(self.num_graphs, 1, i + 1)
            graph.index = i + 1
            graph.setup(axes, probe_data)
            self._displayed_probes[probe_ident] = graph
        
        self._canvas.draw_idle()
    
    def graph_update(self):
        """
        Update the graphs to take new values into account.
        """
        
        updated = False
        for graph in self._displayed_probes.itervalues():
            updated = graph.update() or updated
        
        if updated:
            self._canvas.draw()
        
        return True
    
    def add_probe_value(self, probe, time, value):
        """
        Adds a probe value to the graph.
        """
    
        try:
            self._displayed_probes[probe.global_ident].add_value(time, value)
        except KeyError:
            pass