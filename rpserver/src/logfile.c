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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "rpserver_glob.h"

#define LOGFILE "/tmp/data"

/**
 * Open log file
 */
int logfile_open(sDSWebservice *ds) {
	ds->fd = open(LOGFILE, O_WRONLY | O_APPEND);
	return ds->fd;
}

/**
 * Write log line
 */
int logfile_write(sDSWebservice *ds, const char *str, ...) {
	int ret;
	char str_buf[1024];
	va_list args;
	int printed;

	memset(str_buf, 0, 1024);

	va_start(args, str);
	printed = vsprintf(str_buf, str, args);
	va_end(args);

	if ((ret = write(ds->fd, str_buf, strlen(str_buf))) < 0) {
		LOG(LOG_ERR,"write error %d %s\n", errno, strerror(errno));
	}
	return ret;
}

/**
 * Close log file
 */
int logfile_close(sDSWebservice *ds) {
	int ret;
	ret = close(ds->fd);
	return ret;
}
