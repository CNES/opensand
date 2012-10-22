# -*- coding: utf-8 -*-

"""
probe_display.py - handles the display of probe values on graphs.
"""

from collections import deque
from matplotlib.backends.backend_gtkagg import FigureCanvasGTK
from matplotlib.ticker import FormatStrFormatter
from threading import Lock
import gobject
import matplotlib.pyplot as plt
import random

GRAPH_MAX_POINTS = 30
FORMATTER = FormatStrFormatter('%2.8g')

class ProbeGraph(object):
    def __init__(self, display, program_name, probe_name, unit):
        self._display = display
        self._title = "[%s] %s" % (program_name, probe_name.replace("_", " "))
        
        if unit:
            self._title += " (%s)" % unit
        
        self.index = 0

        self._times = deque(maxlen=GRAPH_MAX_POINTS)
        self._values = deque(maxlen=GRAPH_MAX_POINTS)

        self._axes = None
        self._color = random.choice("bgrcmyk")
        self._dirty = False
    
    def setup(self, axes, direct_values=None):
        """
        Sets up the graph display.
        """
        
        self._axes = axes

        if direct_values is not None:
            x, y = direct_values
        else:
            x = self._times
            y = self._values
        
        axes.set_title(self._title, size="small")
        #axes.tick_params(labelsize="x-small") # Not defined on earlier versions
        axes.get_xaxis().set_major_formatter(FORMATTER)
        axes.get_yaxis().set_major_formatter(FORMATTER)
        axes.plot(x, y, 'o-', color=self._color)
        
        
    def update(self):
        """
        Update the graph display to take new values into account
        """
    
        if not self._dirty:
            return False
    
        self._axes.cla()
        self._axes.set_title(self._title, size="small")
        self._axes.tick_params(labelsize="x-small")
        self._axes.plot(self._times, self._values, 'o-', color=self._color)
        
        self._dirty = False
        
        return True

    def add_value(self, time, value):
        self._times.append(time)
        self._values.append(value)
        
        self._dirty = True
    
    def __str__(self):
        return self._name
    
    def __repr__(self):
        return "<ProbeGraph: %s (%s) index=%d>" % (self._name, self._unit,
            self.index)
    

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
            
            if not graph:
                graph = ProbeGraph(self, probe.program.name, probe.name,
                    probe.unit)
                graph.index = max_index # Temporary, to put it at the end
            
            new_probes.append((probe.global_ident, graph))
        
        new_probes.sort(key=lambda item: item[1].index)
        
        self._displayed_probes = {}
        self._fig.clear()
        
        # Now update the graphs, and construct the new _displayed_probes dict
        self.num_graphs = len(new_probes)
        
        if self.num_graphs == 0:
            return
        
        for i, (probe_ident, graph) in enumerate(new_probes):
            axes = self._fig.add_subplot(self.num_graphs, 1, i + 1)
            graph.index = i + 1
            graph.setup(axes)
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
        try:
            self._displayed_probes[probe.global_ident].add_value(time, value)
        except KeyError:
            pass