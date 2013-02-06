#!/usr/bin/env python2
# -*- coding: UTF8 -*-

# PyGTK Simple Minesweeper 0.3
# (c) 2006 Víctor M. Hernández Rocamora
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
import pygtk
pygtk.require("2.0")
import gtk
import gobject
import cairo
import random
import time

# Constants
SM_MARKUP = '<span font_family="monospace" weight="bold" %s>%s</span>'
SM_NOT_STARTED = SM_MARKUP % ("", ":-)")
SM_IN_GAME = SM_MARKUP % ('foreground="green"', "B-)")
SM_CHECKING = SM_MARKUP % ("", ":-/")
SM_GAME_FAILED = SM_MARKUP % ('foreground="red"', "X-(")
SM_GAME_WON = SM_MARKUP %  ('foreground="blue"', "B-D")
STATUS_NOT_STARTED = 1
STATUS_RUNNING = 2 
STATUS_FINISHED = 3
STATUS_PAUSED = 4
SMALL_OPTIONS = (8,8,10)      # (rows, columns, number of mines)
MIDDLE_OPTIONS = (16,16,40)
LARGE_OPTIONS = (30,16,99)

class MineMap:
    """
    Creates a matrix of randomly positiononed mines.
    self.mine_map[x][y]: contains 1 if at x,y coords there is a mine, 0 otherwise
    self.symbol_map[x][y]: contains a char which will be displayed after (x,y)
         is revealed. This char is '0', '1','2', etc. depending on the number of
         adjacent mines, or "X" if in that position there is a mine.
    self.adjacent_map[x][y]: contains a list of coordinates of adjacent cells.
        i.e. for position (1,1), it will contain:
            (0,0), (1,0), (2,0), 
            (0,1),        (2,1),
            (0,2), (1,2), (2,2)
        It is used when clicking a position with no adjacent mines and
        automatically revealing its adjacent positions.
    """
    def __init__(self, hsize, vsize, mines, clicked_x=-1, clicked_y=-1):
        self.hsize = hsize
        self.vsize = vsize
        self.total_mines = mines
        
        if ( (clicked_x > -1) and    (clicked_y > -1) ) and \
           ( (clicked_x < hsize) and (clicked_y < vsize) ):
            positions = self.__distribute_mines_no_mines_in(clicked_x,clicked_y)
        else:
            positions = self.__distribute_mines()

        self.mine_map = [[0]*vsize for i in xrange(hsize)] # MAP [x][y]
        for pos in positions:
            self.mine_map[pos[0]][pos[1]] = 1
        
        self.symbol_map = [] # map of number of mines in adjacent cells
        self.adjacent_map = [] # map of lists of coordinates of adjacent cells
        for x in range(hsize):
            self.symbol_map.append([])
            self.adjacent_map.append([])
            for y in range(vsize):
                coords = ((x-1,y-1), (x-1, y), (x-1, y+1), 
                          (x,y-1),             (x, y+1),
                          (x+1,y-1), (x+1, y), (x+1, y+1))
                valid_coords = []
                acum = 0
                for i,j in coords:
                    if ((i >= 0) and (i<hsize)) and ((j>=0) and (j<vsize)):
                        valid_coords.append((i,j))
                        acum += self.mine_map[i][j] 
                self.adjacent_map[x].append(valid_coords)
                if self.mine_map[x][y] == 1:
                    self.symbol_map[x].append("X")
                else:
                    self.symbol_map[x].append(acum) 

    def __distribute_mines_no_mines_in(self, cx, cy):
        # build a set with positions that shouldn't contain mines
        nominepositions = set() 
        nominepositions.update(\
            ((cx-1,cy-1), (cx-1, cy), (cx-1, cy+1),
            (cx, cy-1), (cx, cy), (cx,cy+1),
            (cx+1,cy-1), (cx+1, cy), (cx+1,cy+1)))        
        # randomly distribute mines
        positions = set()
        while len(positions) < self.total_mines:
            x = random.randint(0,self.hsize-1)
            y = random.randint(0,self.vsize-1)
            pos = (x, y)
            if not pos in nominepositions:
                positions.add(pos)
        return positions

    def __distribute_mines(self):
        positions = set()
        while len(positions) < self.total_mines:
            x = random.randint(0,self.hsize-1)            
            y = random.randint(0,self.vsize-1)            
            pos = (x, y)
            positions.add(pos)
        return positions


class MineSquare:
    """
    Contains info about the state of one of the postions in the minegrid, as for 
    example its screen coordinates (the coodinates within the widget surface),
    its state(marked, revealed), the label to be displayed once the position is 
    revealed symbol, both background and text color.
    It will call the redraw_cb when the appearance of the square has changed and
    it should be redrawn (i.e. when the square is marked or when it is revealed).
    Calling the redraw_cb with its last paramenter True, will trigger minegrid to
    call reveal_cb in the parent window.
    """
    def __init__(self, x, y, scr_x, scr_y, redraw_cb):
        self.x = x
        self.y = y
        self.redraw_cb = redraw_cb

        self.set_screen_coords(scr_x, scr_y)
        self.reset()

    def set_screen_coords(self, scr_x, scr_y):
        self.scr_x = scr_x
        self.scr_y = scr_y

    def get_screen_coords(self):
        return (self.scr_x, self.scr_y)

    def reset(self):
        self.hiden_label = ""
        self.revealed = False
        self.marked = False
        self.assigned = False
        self.background_colors = (0.8, 0.4, 0.1,
                                  0.5, 0.1, 0.0)
        self.text_color = (0.0,0.0,0.0)
      
    def assign_hiden_label(self, hiden_label):
        self.hiden_label = hiden_label
        self.text_color = self._get_color(self.hiden_label)
        self.assigned = True

    def demark(self):
        if self.marked and (not self.revealed):
            self.marked = False
            self.redraw_cb(self, False)

    def mark(self):
        if (not self.marked) and (not self.revealed):
            self.marked = True
            self.redraw_cb(self, False)

    def show_unmarked_bomb(self):
        if self.revealed or self.marked or (self.hiden_label != "X"): return
        self.revealed = True
        self.marked = False        
        self.redraw_cb(self, False)

    def show_failure(self):
        if self.revealed: return
        self.revealed = True
        self.marked = False       
        self.background_colors = (1.0, 0.2, 0.2,
                                  0.6,  0.1, 0.1)
        self.redraw_cb(self, False)

    def reveal(self):
        if self.revealed or self.marked:  return
        self.revealed = True
        self.background_colors = (1.0, 0.95, 0.95,
                                  0.6,  0.5,  0.5)        
        self.redraw_cb(self, True)

    def _get_color(self, hiden_label):
        if hiden_label == "X":
            return (1.0, 0.0, 0.0)
        elif hiden_label == "1":
            return (0.0, 0.0, 1.0)
        elif hiden_label == "2":
            return (0.0, 1.0, 0.0)
        elif hiden_label == "3":
            return (1.0, 0.0, 0.0)
        elif hiden_label == "4":
            return (0.05, 0.05, 0.5)
        elif hiden_label == "5":
            return (0.5, 0.0, 0.5)
        else:
            return (0.0, 0.2, 0.9)

class MineGrid(gtk.DrawingArea):
    """
    Creation parameters:
        - hsize: number of columns
        - vsize: number of rows
        - reveal_cb: a function to be called when a square has just been
          revealed. The params of this function are 
            -x ...  the coordinates of the  square that has been revealed.
            -y 
          This is used to recursively reveal adjacent square when a square
          without mines in adjacent positions is revealed, to end the
          game either because a square with a mine inside has been revealed by
          the player or when all the squares without mines have been revealed.          
          It is implemented as a direct function cb instead of as a signal in
          order to be executed inmediately after the first square is revealed.
    Signals:
        'clicked': it will be emitted when one of the grids is clicked with 
           either the right or left mouse buttons. The params of the cb are:
            - mouse button: 1 for left button, 3 for right button
            - MineSquare clicked       
    """
    __gtype_name__ = "MineGrid"

    __gsignals__ = {                
        'clicked' : ( gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE,
                         (gobject.TYPE_PYOBJECT,gobject.TYPE_PYOBJECT) ),
        }
    def __init__(self, hsize, vsize, reveal_cb):
        self.hsize = hsize
        self.vsize = vsize
        self.reveal_cb = reveal_cb
        self.minemap = None

        self.currently_pressed = (-1, -1)

        squares = []
        square_list = []
        for x in xrange(self.hsize):
            squares.append([])
            for y in xrange(self.vsize):
                ms = MineSquare(x,y,0,0,self._redraw_cb)
                squares[x].append(ms)
                square_list.append(ms)
        self.squares = squares
        self.square_list = square_list

        gtk.DrawingArea.__init__(self)        
        self.add_events(gtk.gdk.BUTTON_PRESS_MASK | gtk.gdk.BUTTON_RELEASE_MASK)
        self.connect("expose_event", self._expose_cb)
        self.connect("size-allocate", self._size_allocate_cb)
        self.connect("button-press-event", self._button_press_cb)
        self.connect("button-release-event", self._button_release_cb)

    #PUBLIC API
    def assign_map(self, minemap):
        for square in self.square_list:
            square.assign_hiden_label(str(minemap.symbol_map[square.x][square.y]))

    def reset_squares(self):      
        for square in self.square_list:
            square.reset()
        self.queue_draw()       

    def reveal_win(self):
        for square in self.square_list:
            if (not square.revealed) and (not square.marked):
                if square.hiden_label == "X":
                    square.mark()
                else:
                    square.reveal()

    def reveal_fail(self):
        for square in self.square_list:
            if square.hiden_label == "X":
                square.show_unmarked_bomb()
            elif square.marked:
                square.show_failure()

    def reveal_square(self, x, y):
        self.squares[x][y].reveal()

    def is_revealed(self, x, y):
        return self.squares[x][y].revealed

    # CALLBACKS
    def _button_press_cb(self, widget, event, data=None):
        if self._coords_inside_grid(event.x, event.y):
            x = int( (event.x-self.top_coords[0])/self.square_width )
            y = int( (event.y-self.top_coords[1])/self.square_width )
            if self.currently_pressed[0] != -1:                
                cx, cy = self.currently_pressed
                self._redraw_square(self.squares[cx][cy])
            self.currently_pressed = (x, y)
            self._redraw_square(self.squares[x][y])           

    def _button_release_cb(self, widget, event, data=None):
        if self.currently_pressed[0] != -1:
            cx, cy = self.currently_pressed            
            self._redraw_square(self.squares[cx][cy])
            self.currently_pressed = (-1,-1)
            if self._coords_inside_grid(event.x, event.y):
                x = int( (event.x-self.top_coords[0])/self.square_width )
                y = int( (event.y-self.top_coords[1])/self.square_width )
                if (cx == x) and (cy == y):
                    self.emit("clicked",  event.button, self.squares[x][y])

    def _size_allocate_cb(self, widget, allocation, data=None):        
        self.allocation = allocation
        self._calculate_tables()

    def _redraw_cb(self, square, reveal):
        self._redraw_square(square)
        if reveal:
            self.reveal_cb(square.x, square.y)

    def _expose_cb(self, widget, event, data=None):
        self.context = widget.window.cairo_create()
        #set a clip region for the expose event
        self.context.rectangle(event.area.x, event.area.y,
                               event.area.width, event.area.height)
        self.context.clip()
        #configure font settings
        self.context.select_font_face("monospace", cairo.FONT_SLANT_NORMAL,
                                 cairo.FONT_WEIGHT_BOLD)
        self.context.set_font_size(self.square_width*0.8)
        fascent, self.fdescent, self.fheight, fxadvance, fyadvance = \
                self.context.font_extents()
        # detect if we are only going to redraw one square
        if event.area.width <= self.square_width:
            if self._coords_inside_grid(event.area.x, event.area.y):        
                x = int( (event.area.x-self.top_coords[0])/self.square_width )
                y = int( (event.area.y-self.top_coords[1])/self.square_width )
                self._draw_square(self.context, self.squares[x][y])
                return False
        # only redraw squares inside the clip region
        maxx = event.area.x + event.area.width
        maxy = event.area.y + event.area.height
        for square in self.square_list:
            if square.scr_x < maxx and square.scr_y < maxy:
                self._draw_square(self.context, square)
        return False

    #INTERNAL FUNCTIONS        
    def _redraw_square(self, square):
        redraw_x = square.scr_x-self.half_square_width
        redraw_y = square.scr_y-self.half_square_width
        self.queue_draw_area(redraw_x, redraw_y, self.square_width, self.square_width)

    # --- screen coords to grid coords translation functions
    def _calculate_tables(self):
        # the center of our widget
        self.x = self.allocation.width/2
        self.y = self.allocation.height/2
        # width of the rectangle
        if self.allocation.width/self.hsize  < self.allocation.height/self.vsize:
            self.square_width = self.allocation.width/self.hsize
        else:
            self.square_width = self.allocation.height/self.vsize
        self.half_square_width = self.square_width / 2
        self.top_coords = (self.x-(self.half_square_width*self.hsize),
                           self.y-(self.half_square_width*self.vsize))
        self.bottom_coords = (self.x+(self.half_square_width*self.hsize),
                              self.y+(self.half_square_width*self.vsize))
        for square in self.square_list:
            cx = self.top_coords[0]+(self.square_width*square.x)+self.half_square_width
            cy = self.top_coords[1]+(self.square_width*square.y)+self.half_square_width
            square.set_screen_coords(cx,cy)

    def _coords_inside_grid(self, x, y):
        return ((x >= self.top_coords[0]) and (y >= self.top_coords[1])) and \
               ((x <= self.bottom_coords[0]) and (y <= self.bottom_coords[1]))

    # --- grid drawing functions
    def _draw_letter(self, context, letter, cx, cy, color):
        r, g, b = color
        context.set_source_rgb(r,g,b)
        # font settings have been set previously in self._expose_cb
        xbearing, ybearing, width, height, xadvance, yadvance = \
                context.text_extents(letter)
        context.move_to(cx + 0.5 - xbearing - width / 2,
                        cy + 0.5 - self.fdescent + self.fheight / 2)
        context.show_text(letter)

    def _draw_rectangle(self, context, x, y, width, margin, curve1, curve2):
        ll = width-(width*margin)
        h_ll = -ll/2
        l5pc = ll*curve1
        l2pc = ll*curve2
        sll = ll - (l5pc*2)
        context.move_to(x,y)
        context.rel_move_to( l5pc+h_ll,h_ll)
        context.rel_line_to(sll, 0)
        context.rel_curve_to( l2pc, 0, l5pc, l2pc, l5pc, l5pc)
        context.rel_line_to(0, sll)
        context.rel_curve_to( 0, l2pc, -l2pc, l5pc, -l5pc, l5pc)
        context.rel_line_to(-sll, 0)
        context.rel_curve_to( -l2pc, 0, -l5pc , -l2pc, -l5pc, -l5pc)
        context.rel_line_to(0,-sll)
        context.rel_curve_to( 0, -l2pc, l2pc , -l5pc, l5pc, -l5pc)

    def _draw_square(self, context, square):
        x = square.x;   y = square.y
        cx = square.scr_x; cy = square.scr_y
        #draw the rectangle:
        context.set_line_width(1.0)
        self._draw_rectangle(context, cx, cy, self.square_width, 0.01, 0.1, 0.1)
        #fill the rectangle with a gradient:
        lg = cairo.LinearGradient(cx+self.half_square_width, cy,
                                  cx+self.half_square_width, cy+self.square_width)
        if (x == self.currently_pressed[0]) and (y == self.currently_pressed[1]):
            lg.add_color_stop_rgb(0.0, 0.7, 0.7, 0.7)
            lg.add_color_stop_rgb(1.0, 0.3, 0.3, 0.3) 
        else:
            r1, g1, b1, r2,g2,b2 = square.background_colors
            lg.add_color_stop_rgb(0.0, r1, g1, b1)
            lg.add_color_stop_rgb(1.0, r2, g2, b2)
        context.set_source(lg)
        context.fill_preserve()
        # Draw square text
        if square.marked:    
            self._draw_letter(context, 'M', cx, cy, (0.0,0.0,0.0))
        elif square.revealed and square.hiden_label != '0':
            self._draw_letter(context, square.hiden_label, 
                                cx, cy, square.text_color)                    
        #set exterior line color
        context.set_source_rgb(0.3,0.3,0.3)
        context.stroke()


class MineWindow(gtk.Window):
    """
    The main window of the game. It contains:
    - self.minegrid: a MineGrid widget
    - self.start_button: a button whose label (self.sb_label) will change with
         the status of the game and that restarts the game when pressed.
    - two labels, one indicating the time since the match started, and one 
      showing the amount of squares marked and the amount of mines hiden in the 
      grid.
    - when the window loses the focus, the game is paused. The minegrid is then
      hiden and a resume button appears. Besides, the timer measuring the time
      since the match started also is stopped.
    """
    def __init__(self, hsize, vsize, mines):
        self.hsize = hsize
        self.vsize = vsize
        self.total_mines = mines  
        self.boxes_without_mines = (hsize*vsize)-mines

        self._create_widgets()
    
        self.reset_game()

    # Functions affecting the gameflow (restart, pause, etc...)
    def reset_game(self):
        self.minemap = None
        self.marked_squares = 0
        self.correctly_revealed_boxes = 0
        self._actualize_mine_counter()        
        self.sb_label.set_markup(SM_NOT_STARTED)
        self.minegrid.reset_squares() 
        self.game_status = STATUS_NOT_STARTED        
        self.timer_on = False
        self.resume_hbox.hide()
        self.minegrid.show()

    def pause_game(self):
        self.acumulated_time += (time.time() - self.start_time)
        self.timer_on = False
        self.resume_hbox.show()
        self.minegrid.hide()                
        self.game_status = STATUS_PAUSED

    def resume_game(self):        
        self.resume_hbox.hide()
        self.minegrid.show()
        self.start_time = time.time()
        self.timer_on = True        
        gobject.timeout_add(1000, self._timer_cb)
        self.game_status = STATUS_RUNNING

    def hint(self):        
        #squares without mines in adjacent positions:
        free_positions_no_mines = [] 
        #squares without a hiden mine, but with mines in adjacent squares:
        free_positions_mines = [] 
        for x in xrange(self.hsize):
            for y in xrange(self.vsize):                
                if (self.minemap.mine_map[x][y] == 0) and \
                   (not self.minegrid.is_revealed(x,y)):
                    if self.minemap.symbol_map[x][y] == 0:
                        free_positions_no_mines.append((x,y))
                    elif self.minemap.symbol_map[x][y] != "X":
                        free_positions_mines.append((x,y))
        # randomly reveal one of the square without hiden mines
        # we'll reveal first squares with adjacent mines and when we run out of
        # those, squares without adjacent mines.
        if len(free_positions_mines)>0:
            x,y = random.choice(free_positions_mines)
            self.acumulated_time += 10.0 #penalization for using hints!
            self.minegrid.reveal_square(x,y)
        elif len(free_positions_no_mines)>0:
            x,y = random.choice(free_positions_no_mines)
            self.acumulated_time += 10.0 
            self.minegrid.reveal_square(x,y)

    # CALLBACKS
    def _start_clicked_cb(self, widget, *args, **kargs):
        self.reset_game()        

    def _square_reveal_cb(self, x, y):
        if self.minemap.mine_map[x][y] == 1:
            self._fail_game()
        else:
            self.correctly_revealed_boxes += 1
            if self.correctly_revealed_boxes == self.boxes_without_mines:
                self._win_game()
                return
            if self.minemap.symbol_map[x][y] == 0:
                for i,j in self.minemap.adjacent_map[x][y]:
                    if not self.minegrid.is_revealed(i,j):
                        self.minegrid.reveal_square(i,j)

    def _start_game(self, clicked_x, clicked_y):
        self.minemap = MineMap(self.hsize, self.vsize, self.total_mines, 
                               clicked_x, clicked_y)
        self._actualize_mine_counter()
        self.minegrid.assign_map(self.minemap)
        self.sb_label.set_markup(SM_IN_GAME)
        self.game_status = STATUS_RUNNING
        self._start_timer()

    def _win_game(self):
        self._stop_timer()
        self.timer_on = False
        self.game_status = STATUS_FINISHED
        self.minegrid.reveal_win()        
        self.sb_label.set_markup(SM_GAME_WON)
        self.resume_hbox.hide()
        # calculate elapsed time and actualize time label
        duration = time.time() - self.start_time + self.acumulated_time
        t = "%02d:%02d:%02d" % self._time_interval_to_hms(duration)
        self.time_label.set_markup("<b>%s</b>" % t)
            
    def _fail_game(self):
        self._stop_timer()
        self.game_status = STATUS_FINISHED
        self.minegrid.reveal_fail()                
        self.sb_label.set_markup(SM_GAME_FAILED)
        self.resume_hbox.hide()

    def _square_clicked_cb(self, widget, mouse_button, square):        
        if square.revealed: return
        if mouse_button == 1:
            if self.game_status == STATUS_RUNNING:                
                if not square.marked:
                    square.reveal()
            elif self.game_status == STATUS_NOT_STARTED:
                self._start_game(square.x, square.y)
                square.reveal()
        elif (mouse_button == 3) and (self.game_status == STATUS_RUNNING):
            if square.marked:
                square.demark()
                self.marked_squares -= 1                
            else:
                square.mark()
                self.marked_squares += 1                
            self._actualize_mine_counter() 

    def _actualize_mine_counter(self):
        self.mine_label.set_markup("<b>Marked: %i / Total: %i</b>" % 
                    (self.marked_squares, self.total_mines))

    def _time_interval_to_hms(self, duration):
        hours = duration / 3600
        if hours >= 1.0:
            mins = hours % 3600
            secs = mins % 60             
        else:
            mins = duration / 60
            if mins >= 1.0:
                secs = duration % 60
            else:
                secs = duration
        return (int(hours), int(mins), int(secs))

    def _resume_button_clicked_cb(self, widget):
        self.resume_game()

    def _pause_accel_cb(self, widget, *args, **kargs):
        if (self.game_status == STATUS_RUNNING) and self.timer_on:
            self.pause_game()
        
    def _hint_accel_cb(self, accel_group, acceleratable, keyval, modifier):
        if self.game_status == STATUS_RUNNING:
            self.hint()

    def _start_timer(self):
        if not self.timer_on:
            self.start_time = time.time()
            self.acumulated_time = 0.0
            self.timer_on = True
            gobject.timeout_add(1000, self._timer_cb)        
     
    def _stop_timer(self):
        if self.timer_on:            
            self.timer_on = False

    def _timer_cb(self):
        if self.timer_on:
            duration = time.time() - self.start_time + self.acumulated_time
            t = "%02d:%02d:%02d" % self._time_interval_to_hms(duration)
            self.time_label.set_markup("<b>%s</b>" % t)
        return self.timer_on

    def _create_widgets(self):
        gtk.Window.__init__(self)
        vbox = gtk.VBox()
        # Start/status button
        hbox = gtk.HBox()
        self.sb_label = gtk.Label()
        self.sb_label.set_markup(SM_NOT_STARTED)
        self.sb_label.set_angle(270)
        self.start_button = gtk.Button()
        self.start_button.add(self.sb_label)
        self.start_button.set_size_request(80,50)        
        self.start_button.connect("clicked", self._start_clicked_cb)
        hbox.pack_start(self.start_button, True, False, 10)
        vbox.pack_start(hbox, False, False, 0)
        # Minegrid widget
        self.minegrid = MineGrid(self.hsize, self.vsize, self._square_reveal_cb)
        self.minegrid.connect("clicked", self._square_clicked_cb)
        vbox.pack_start(self.minegrid, True, True, 0)
        # Resume button
        resume_button = gtk.Button("Resume game")
        resume_button.connect("clicked", self._resume_button_clicked_cb)
        self.resume_hbox = gtk.HBox()
        self.resume_hbox.pack_start(resume_button, True, False, 0)        
        vbox.pack_start(self.resume_hbox, True, False, 0)
        # Labels showing elapsed time and marked boxes)
        labels_hbox = gtk.HBox()
        self.time_label = gtk.Label()
        self.time_label.set_markup("<b>00:00:00</b>")        
        self.mine_label = gtk.Label()
        labels_hbox.pack_start(self.time_label, False, False, 10)
        labels_hbox.pack_end(self.mine_label, False, False, 10)
        vbox.pack_end(labels_hbox, False, False, 0)
        # Pack widgets and create window conections
        self.add(vbox)
        self.set_default_size(500,500)
        self.show_all()    
        self.connect("focus-out-event", self._pause_accel_cb)
        # Set key bindings
        ag = gtk.AccelGroup()       
        ag.connect_group( gtk.gdk.keyval_from_name("P"),
                          gtk.gdk.CONTROL_MASK, gtk.ACCEL_LOCKED, 
                          self._pause_accel_cb)
        ag.connect_group( gtk.gdk.keyval_from_name("H"),
                          gtk.gdk.CONTROL_MASK, gtk.ACCEL_LOCKED, 
                          self._hint_accel_cb)
        ag.connect_group( gtk.gdk.keyval_from_name("N"),
                          gtk.gdk.CONTROL_MASK, gtk.ACCEL_LOCKED, 
                          self._start_clicked_cb)
        self.add_accel_group(ag)



class SizeDialog(gtk.Dialog):
    def __init__(self):
        gtk.Dialog.__init__(self)
        self.small_radio = gtk.RadioButton(label="Small")
        self.middle_radio = gtk.RadioButton(group=self.small_radio,
                                            label="Middle")
        self.large_radio = gtk.RadioButton(group=self.small_radio,
                                            label="Large")
        self.vbox.pack_start(self.small_radio, False, False, 0)
        self.vbox.pack_start(self.middle_radio, False, False, 0)
        self.vbox.pack_start(self.large_radio, False, False, 0)
        self.add_button(gtk.STOCK_APPLY, gtk.RESPONSE_ACCEPT)
        self.small_radio.set_active(True)
        self.size_options = SMALL_OPTIONS
        self.small_radio.connect("toggled", self._radio_toggled_cb, "S")
        self.middle_radio.connect("toggled", self._radio_toggled_cb, "M")
        self.large_radio.connect("toggled", self._radio_toggled_cb, "L")
        self.show_all()

    def _radio_toggled_cb(self, widget, option):
        if not widget.get_active(): return
        if option == "S":
            self.size_options = SMALL_OPTIONS
        elif option == "M":
            self.size_options = MIDDLE_OPTIONS
        elif option == "L":
            self.size_options = LARGE_OPTIONS                

def delete_event_cb(widget, *args, **kargs):
    gtk.main_quit()



def main():
    d = SizeDialog()
    d.run()
    h,v,n = d.size_options
    d.destroy()
    w = MineWindow(h,v,n)
    w.connect("delete_event", delete_event_cb)
    gtk.main()        

if __name__=="__main__":
    main()
