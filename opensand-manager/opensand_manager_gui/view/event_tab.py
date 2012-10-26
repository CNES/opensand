# -*- coding: utf-8 -*-

"""
event_tab.py - Used to handles event tabs
"""

from opensand_manager_core.model.environment_plane import EventLevel
import gobject
import gtk
import time


class EventTab(object):
    """
    Handler for a tab used to display events
    """

    def __init__(self, notebook, title):
        self._notebook = notebook
        self._icon_level = -1
    
        # Tab
        self._act_image = gtk.Image()
        self._act_image.hide()
        
        tab_label = gtk.Label(title)
        tab_label.show()
        
        tab_hbox = gtk.HBox()
        tab_hbox.pack_start(self._act_image)
        tab_hbox.pack_start(tab_label)
        tab_hbox.show()
        
        # Content
        self._buff = gtk.TextBuffer()
        red = self._buff.create_tag('red')
        red.set_property('foreground', 'red')
        orange = self._buff.create_tag('orange')
        orange.set_property('foreground', 'orange')
        green = self._buff.create_tag('green')
        green.set_property('foreground', 'green')
        
        self._text = gtk.TextView()
        self._text.set_editable(False)
        self._text.set_wrap_mode(gtk.WRAP_CHAR)
        self._text.set_cursor_visible(False)
        self._text.set_buffer(self._buff)
        self._text.show()
        
        hadjustement = gtk.Adjustment(value=0, lower=0, upper=100, step_incr=1,
            page_incr=10, page_size=10)
        vadjustement = gtk.Adjustment(value=0, lower=0, upper=100, step_incr=1,
            page_incr=10, page_size=10)
        
        scroll = gtk.ScrolledWindow(hadjustment=hadjustement,
            vadjustment=vadjustement)
        scroll.set_policy(gtk.POLICY_NEVER, gtk.POLICY_ALWAYS)
        scroll.add(self._text)
        scroll.set_size_request(-1, 150)
        scroll.show()
        
        self._page_num = notebook.append_page(scroll, tab_hbox)
        notebook.connect("switch-page", self._check_switch_page)
    
    def message(self, severity, identifier, text):
        """
        Adds a message to the event tab
        """
    
        now = time.localtime()
        gobject.idle_add(self._message, now, severity, identifier, text)
    
    def _message(self, date, severity, identifier, text):
        """
        Internal handler to add a message to the event tab
        """
    
        at_end = self._buff.get_end_iter
        
        if severity == EventLevel.ERROR:
            color = 'red'
            sev_text = "ERROR"
            image = gtk.STOCK_DIALOG_ERROR
            
        elif severity == EventLevel.WARNING:
            color = 'orange'
            sev_text = "WARNING"
            image = gtk.STOCK_DIALOG_WARNING
        
        elif severity == EventLevel.INFO:
            color = 'green'
            sev_text = "INFO"
            image = gtk.STOCK_DIALOG_INFO
            
        else:
            color = 'green'
            sev_text = "DEBUG"
            image = gtk.STOCK_DIALOG_INFO
        
        self._buff.insert(at_end(), time.strftime("%H:%M:%S ", date))
        if identifier:
            self._buff.insert(at_end(), "%s: " % identifier)
        self._buff.insert_with_tags_by_name(at_end(), "[%s]: " % sev_text,
            color)
        self._buff.insert(at_end(), text + "\n")
        self._buff.place_cursor(at_end())
        self._text.scroll_to_mark(self._buff.get_insert(), 0.0, False, 0, 0)
        
        if self._notebook.get_current_page() != self._page_num:
            if self._icon_level < severity:
                self._icon_level = severity
                self._act_image.set_from_stock(image, gtk.ICON_SIZE_MENU)
        
            self._act_image.show()        
        
        return False
    
    def _check_switch_page(self, _notebook, _page, page_num):
        """
        Called when the notebook page is changed
        """
    
        if page_num == self._page_num:
            self._act_image.hide()
            self._icon_level = -1