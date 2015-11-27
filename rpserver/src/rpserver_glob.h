/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2012 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef RPSERVER_GLOB_H_
#define RPSERVER_GLOB_H_



#define LOGMASK LOG_MASK(LOG_DEBUG)

#define uuDEBUG
//#define LOGFUNC
#ifdef LOGFUNC
#define LOG(prio, ...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(prio, ...) syslog(prio, __VA_ARGS__)
#endif

#define DB_ENSERVER


#define DEFAULT_FILEPUT_DIR "/tmp/rpservertx"
#define DEFAULT_FILEGET_DIR "/tmp/rpserverrx"


#endif /* RPSERVER_GLOB_H_ */
