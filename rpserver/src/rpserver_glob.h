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

enum access_func {
	REMACC_BOX = 100,
	REMACC_BOX_INFO,
	REMACC_BOX_MEAS,
	REMACC_BOX_META,
	REMACC_BOX_CMDUP,
	REMACC_BOX_FILEG,
	REMACC_BOX_FILES,
	REMACC_BOX_PAIR,
	REMACC_DATA = 200,
	REMACC_DATA_ETYPE,
	REMACC_DATA_EVENTS,
	REMACC_CMD = 300,
	REMACC_CMD_LISTGET,
	REMACC_CMD_ADD,
	REMACC_CMD_GET, // cmdGet
	REMACC_CMD_SEARCH, //cmdSearch
	REMACC_CMD_STATUS, //cmdStatus
	REMACC_CMD_DELETE,
//cmdDelete
};

typedef struct {
	//dbi_conn db;
	int fd;
	int drivercount;
	int iid;
	int boxid;
	int user;
	int acclev;
	int dbglev;
	enum access_func func;
} sDSWebservice;

enum boxcmdsta {
	BOXCMDSTA_NONE = -1, BOXCMDSTA_EMPTY = 0, BOXCMDSTA_QUEUED = 1, /**< The command is set in queue */
	BOXCMDSTA_SEND, /**< The command has been send to box */
	BOXCMDSTA_EXECUTING, /**< The command has been executed on box */
	BOXCMDSTA_EXECUTED, /**< The command has been executed on box */
	BOXCMDSTA_DELETIG, /**< The command is marked for deleting on box */
	BOXCMDSTA_DELETED, /**< The command has been deleted on box */
	BOXCMDSTA_EXECDEL, /**< The commend has been marked for
	 deletion but has allready been executed on box */
	BOXCMDSTA_EXECERR, /**< An error while executing the command */
	BOXCMDSTA_ERROBS, /**< An newer command of the same type is executed */
	BOXCMDSTA_UNKNOWN, /**< An unknown state */
	BOXCMDSTA_SENDING, /**< An sending state */
	BOXCMDSTA_MAX,
/**< An unknown state */
};

struct rembox_cmd {
	unsigned int device_id;
	unsigned int cmd_id;
	char cmd[32];
	char* param;
	int user;
	time_t ttime;
	time_t stime;
	time_t etime;
	enum boxcmdsta status;
	char *retval;
	int pseq;
	struct rembox_cmd *next;
};

#endif /* RPSERVER_GLOB_H_ */
