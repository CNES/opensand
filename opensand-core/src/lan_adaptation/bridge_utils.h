/*
 * Copyright (C) 2000 Lennert Buytenhek
 * Copyright (C) 2017 CNES
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
 */


/**
 * @file bridge_utils.h
 * @brief useful functions taken from libbridge
 *
 * Some functions of the bridge library (from brctl) used to
 * create a bridge in OpenSAND
 */

#ifndef BRIDGE_UTILS_H
#define BRIDGE_UTILS_H

int br_add_interface(const char *bridge, const char *dev);

int br_del_interface(const char *bridge, const char *dev);

int br_add_bridge(const char *brname);

int br_del_bridge(const char *brname);

int br_init(void);

void br_shutdown(void);

int set_if_flags(int fd, const char *ifname, short flags);

int set_if_up(int fd, const char *ifname, short flags);

int set_if_down(int fd, const char *ifname, short flags);

#endif

