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
#include <stdlib.h>
#include <dirent.h>
#include "rpserver_glob.h"

#define TMPCMD_DIR "/tmp/cmd/"

/**
 * Add new command to linked list
 */
struct rembox_cmd* rembox_cmd_add(struct rembox_cmd* list,
		struct rembox_cmd* new) {
	struct rembox_cmd* ptr = list;

	if (!list)
		return new;

	while (ptr->next)
		ptr = ptr->next;

	ptr->next = new;

	return list;
}

/**
 * Create new command
 */
struct rembox_cmd* rembox_cmd_create(unsigned int device_id,
		unsigned int cmd_id, const char *cmd, const char* param, int user,
		time_t ttime, time_t stime, time_t etime, enum boxcmdsta status,
		char *retval, int pseq) {
	struct rembox_cmd *new = malloc(sizeof(*new));

	memset(new, 0, sizeof(*new));

	new->device_id = device_id;
	new->cmd_id = cmd_id;

	strncpy(new->cmd, cmd, sizeof(new->cmd));
	if (param)
		new->param = strdup(param);

	new->user = user;
	new->ttime = ttime;
	new->stime = stime;
	new->etime = etime;

	new->status = status;

	if (retval)
		new->retval = strdup(retval);

	new->pseq = pseq;

	return new;

}

/**
 * Look in TMPCMD_DIR for new command files, and add them to the cmd queue
 */
struct rembox_cmd* cmd_xmitist(struct boxInfo *boxInf, int *count_) {
	struct rembox_cmd* list = NULL;
	int count = 0;
	char *strptr;
	char *param;
	char *value;
	int fd;

	LOG(LOG_DEBUG,"%s():%d\n",__FUNCTION__, __LINE__);

	strptr = malloc(100);
	param = malloc(25);
	value = malloc(10);

	DIR *dp;
	struct dirent *ep;

	dp = opendir(TMPCMD_DIR);
	if (dp != NULL) {
		while (ep = readdir(dp)) {
			if ((0 > strcmp(".", ep->d_name)) && (0 > strcmp("..", ep->d_name))) {
				LOG(LOG_DEBUG,"dirent: ep->d_name = %s\n",ep->d_name);
				memset(strptr, 0, 100);
				strcpy(strptr, TMPCMD_DIR);
				strcat(strptr, ep->d_name);
				LOG(LOG_DEBUG,"%s\n",strptr);
				fd = fopen(strptr, "r");
				if (fd < 0) {
					LOG(LOG_DEBUG,"Error opening file\n");
				}
				memset(param, 0, 25);
				memset(value, 0, 10);
				fscanf(fd, " %[^ ]s", param);
				fscanf(fd, "%s", value);
				/* CMD START */
				struct rembox_cmd* cmd = rembox_cmd_create(1, atoi(ep->d_name),
						param, value, 0, time(NULL), time(NULL), 0,
						BOXCMDSTA_QUEUED, NULL, 0);

				LOG(LOG_INFO, "%s():%d - result: %d: %d, %p \n", __FUNCTION__,
						__LINE__, count, 0, cmd);

				list = rembox_cmd_add(list, cmd);
				count++;
				/* CMD END */

				fclose(fd);
				if (-1 == remove(strptr))
					LOG(LOG_INFO, "Error deleting file = %s\n",strptr);
			}
		}
		(void) closedir(dp);
	} else
		LOG(LOG_DEBUG,"Couldn't open the directory");

	if (count_)
		*count_ = count;

	return list;
}
