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

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/queue.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include <stdsoap2.h>

#include <soapH.h>	/* include generated proxy and SOAP support */
#include "liabdatalogger.nsmap"
#include "rpserver.h"

#include "rpserver_glob.h"
#include "logfile.h"
#include "cmdfile.h"

enum cmdsta rpserver_cmd_fromremdb(enum boxcmdsta status);
enum boxcmdsta rpserver_cmd_toremdb(enum cmdsta status);

/**
 * This is a CGI bin program for recieving gSOAP messages from LIAB/Energinet datalogger
 */
int main(int argc, char *argv[]) {
	struct soap soap;
	sDSWebservice *ds;
	SOAP_SOCKET m, s; /* master and slave sockets */
	int ret = 0;

	openlog("rpserver", LOG_PID, LOG_USER);

	/* Choose between Warning log or Debug log */
	setlogmask(LOG_UPTO(LOG_WARNING));
	//setlogmask(LOG_UPTO(LOG_DEBUG));

	LOG(LOG_INFO, "%s():%d - start (last change %s)\n", __FUNCTION__, __LINE__, __TIMESTAMP__);

	ds = (sDSWebservice *) calloc(1, sizeof(sDSWebservice));

	ds->iid = -1;
	ds->boxid = -1;

	/* Open log file */
	if (logfile_open(ds) < 0) {
		LOG(LOG_INFO, "%s():%d - error opening logfile", __FUNCTION__, __LINE__);
		return SOAP_FATAL_ERROR;
	}

	soap_init1(&soap, SOAP_C_UTFSTRING | SOAP_IO_STORE );

	/* Assign user data */
	soap.user = (void *) ds;

	if (argc < 2) {
		/****************************************************/
		/* CGI                                              */
		/****************************************************/
		LOG(LOG_DEBUG, "%s():%d - called as CGI", __FUNCTION__, __LINE__);
		ret = soap_serve(&soap); /* serve as CGI application */
		if (ret != 0) {
			soap_print_fault(&soap, stderr); // print error
			LOG(LOG_ERR, "%s():%d - Error. soap_serve returned %d", __FUNCTION__, __LINE__, ret);
		}
		soap_end(&soap);
	} else {
		/****************************************************/
		/* STAND-ALONE                                      */
		/****************************************************/

		m = soap_bind(&soap, NULL, atoi(argv[1]), 100);
		if (!soap_valid_socket(m)) {
			soap_print_fault(&soap, stderr);
			ret = -1;
			goto out;
		}
		fprintf(stderr, "Socket connection successful: master socket = %d\n", m);
		for (;;) {
			s = soap_accept(&soap);
			fprintf(stderr,
					"Socket connection successful: slave socket = %d\n", s);
			if (!soap_valid_socket(s)) {
				soap_print_fault(&soap, stderr);
				ret = -1;
				goto out;
			}
			soap_serve(&soap);
			soap_end(&soap);
		}
	}

	char scriptaddr[256];

	snprintf(scriptaddr, sizeof(scriptaddr), "http%s://%s%s", (atoi(getenv(
			"SERVER_PORT")) == 443) ? "s" : "", getenv("SERVER_NAME"), getenv(
			"SCRIPT_NAME"));

	out: logfile_close(ds);
	free(ds);

	LOG(LOG_INFO, "%s():%d - done serving...", __FUNCTION__, __LINE__);

	closelog();

	return ret;
}

/**
 * Basic authentication
 */
int check_basic_auth(struct soap *soap) {
	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;

	if (!soap->userid || !soap->passwd) {
		printf("Setting authrealm\n");
		soap->authrealm = "Mikkels realm";
		return 401;
	} else {
		return 0;
	}
}

/*
 * Posix time to human readable time string
 */
char *time_fromstr(const char *timestr) {
	int tim_sec;
	char *buf;
	char timebuf[1024];
	struct tm *tv;

	if (!timestr)
		return -1;

	if (sscanf(timestr, "%ld", &tim_sec) != 1)
		return -1;

	time_t sec = (time_t) tim_sec;
	tv = localtime(&sec);
	strftime(timebuf, 1024, "%c", tv);
	buf = strdup(timebuf);

	return buf;
}

/******************** External API ********************/
int liabdatalogger__getApiVersion(struct soap *soap,
		struct apiVersion *versionResp) {
	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;

	LOG(LOG_DEBUG, "%s():%d - start", __FUNCTION__, __LINE__);

	versionResp->description = strdup("Energinet Datalogger WebService");
	versionResp->major = 0;
	versionResp->minor = 0;
	versionResp->sub = 1;

	LOG(LOG_DEBUG, "%s():%d - end", __FUNCTION__, __LINE__);
	return SOAP_OK;
}

int liabdatalogger__statusTelegram(struct soap *soap, struct status *status,
		struct signalResponse *out) {
	int retval = SOAP_ERR;

	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;

	/* Validate user */
	LOG(LOG_DEBUG, "%s():%d - start", __FUNCTION__, __LINE__);

	LOG(LOG_DEBUG, "%s():%d - statusType    : %s", __FUNCTION__, __LINE__, status->type);
	LOG(LOG_DEBUG, "%s():%d - statusName    : %s", __FUNCTION__, __LINE__, status->name);
	LOG(LOG_DEBUG, "%s():%d - statusContent : %s", __FUNCTION__, __LINE__, status->value);

	retval = SOAP_OK;

	LOG(LOG_DEBUG, "%s():%d - end", __FUNCTION__, __LINE__);
	return retval;
}

/**
 * Set command
 */
void boxCmdSet(struct boxCmd *cmd, const char *id, int sequence, int pseq,
		unsigned long trigger_time, int status, int param_cnt, ...) {

	va_list ap;
	int i;

	LOG(LOG_DEBUG, "%s():%d %s %d %d\n", __FUNCTION__, __LINE__, id, sequence, pseq );

	cmd->name = strdup(id);
	cmd->sequence = sequence;
	cmd->pseq = pseq;
	cmd->params = malloc(sizeof(void*) * param_cnt);
	cmd->paramsCnt = param_cnt;
	cmd->status = status;
	{
		char ttime[32];
		sprintf(ttime, "%ld", trigger_time);
		cmd->trigtime = strdup(ttime);
	}
	va_start (ap, param_cnt);
	for (i = 0; i < param_cnt; i++) {
		cmd->params[i] = strdup(va_arg(ap,const char *));
	}
	va_end(ap);
}

/**
 * Fill commandlist for box
 */
void boxCmdFill(struct boxCmd *cmd, int maxcnt, struct rembox_cmd* list) {

	if (list == NULL || maxcnt < 0) {
		return;
	}

	boxCmdSet(cmd, list->cmd, list->cmd_id, list->pseq, list->ttime,
			list->status, 1, list->param);

	boxCmdFill(cmd + 1, maxcnt - 1, list->next);
}

/**
 * Delete command
 */
void cmd_delete(struct rembox_cmd* cmd) {
	if (!cmd) {
		return;
	}

	cmd_delete(cmd->next);

	free(cmd->param);
	free(cmd->retval);
	free(cmd);
}

/**
 * Check for new commands
 */
void fillHostRet(struct boxInfo *boxInf, struct hostReturn *hostRet) {
	int cmd_cnt, i;

	struct rembox_cmd* cmdlist = cmd_xmitist(boxInf, &cmd_cnt);

	hostRet->cmds = malloc(sizeof(struct boxCmd) * cmd_cnt);
	hostRet->cmdCnt = cmd_cnt;
	boxCmdFill(hostRet->cmds, cmd_cnt, cmdlist);

	cmd_delete(cmdlist);

	for (i = 0; i < hostRet->cmdCnt; i++) {
		struct boxCmd *cmd = hostRet->cmds + i;
		LOG(LOG_DEBUG, "%s():%d - cmd: %d \"%s\"  %p\n", __FUNCTION__, __LINE__, i, cmd->name, cmd);
	}

	if (hostRet->cmdCnt == 0)
		LOG(LOG_DEBUG, "%s():%d - cmd count == 0\n", __FUNCTION__, __LINE__);

	return;
}

/**
 * Contact from client
 */
int rpserver_box_contact(sDSWebservice *ds, struct boxInfo *boxInf,
		struct hostReturn *hostRet) {
	char externalip[32];
	char scriptaddr[256];
	sprintf(externalip, "%s", getenv("REMOTE_ADDR"));
	snprintf(scriptaddr, sizeof(scriptaddr), "http://%s%s", getenv(
			"SERVER_NAME"), getenv("SCRIPT_NAME"));

	ds->boxid = 1;

	if (ds->boxid < 0) {
		LOG(LOG_DEBUG, "%s():%d - error getting device_id for '%s' : %d \n",
				__FUNCTION__, __LINE__, boxInf->name, ds->boxid);
		return -1;
	}

	if (hostRet)
		fillHostRet(boxInf, hostRet);

	return ds->boxid;
}

int liabdatalogger__sendBoxInfo(struct soap *soap, struct boxInfo *boxInf,
		struct hostReturn *hostRet) {

	int i;
	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;
	struct timeval tv;

	ds->func = REMACC_BOX_INFO;

	LOG(LOG_DEBUG, "box type: %s , name %s \n", boxInf->type, boxInf->name);
	fillHostRet(-1, hostRet);

	for (i = 0; i < hostRet->cmdCnt; i++) {
		struct boxCmd *cmd = hostRet->cmds + i;
		LOG(LOG_DEBUG, "cmd: %d \"%s\"  %p\n", i, cmd->name, cmd);
	}

	gettimeofday(&tv, NULL);

	return SOAP_OK;
}

/**
 * Receive logs
 */
int liabdatalogger__sendMeasurments(struct soap *soap,
		struct measurments* meas, struct hostReturn *hostRet) {
	sDSWebservice *ds = (sDSWebservice *) soap->user;
	struct boxInfo *boxInf = &meas->boxInf;
	int i, n;

	ds->func = REMACC_BOX_MEAS;

	LOG(LOG_DEBUG, "%s():%d - seriesCnt = %d\n", __FUNCTION__, __LINE__,meas->seriesCnt);

	if (rpserver_box_contact(ds, boxInf, hostRet) < 0)
		return SOAP_ERR;

	for (i = 0; i < meas->seriesCnt; i++) {
		struct eventSeries *eseries = meas->series + i;
		struct timeval tv_end;
		for (n = 0; n < eseries->measCnt; n++) {
			struct eventItem *eitem = eseries->meas + n;
			char *timebuf;
			timebuf = time_fromstr(eitem->time);

			LOG(LOG_DEBUG, "Log: %s %s - %s",eseries->ename,timebuf,eitem->value);
			logfile_write(ds, "%s %s - value %s\n", eseries->ename, timebuf,
					eitem->value);
		}
	}

	return SOAP_OK;

}

int liabdatalogger__sendMetadata(struct soap *soap, struct measMeta* metaData,
		struct hostReturn *hostRet) {
	int i;
	sDSWebservice *ds = (sDSWebservice *) soap->user;
	struct boxInfo *boxInf = &metaData->boxInf;

	ds->func = REMACC_BOX_META;

	LOG(LOG_DEBUG, "%s():%d\n", __FUNCTION__, __LINE__);

	if (rpserver_box_contact(ds, boxInf, hostRet) < 0)
		return SOAP_ERR;

	for (i = 0; i < metaData->etypeCnt; i++) {
		struct eTypeMeta *etype = metaData->etypes + i;
	}

	return SOAP_OK;
}

/**
 * Send command status update 
 * @param update, command that has been update
 * @param retval it the cid in the rembox database
 */
int liabdatalogger__cmdUpdate(struct soap *soap, struct boxCmdUpdate *update,
		int *retval) {
	int status;
	sDSWebservice *ds = (sDSWebservice *) soap->user;
	int cid = update->sequence;
	struct boxInfo *boxInf = &update->boxInf;

	ds->func = REMACC_BOX_CMDUP;

	if (rpserver_box_contact(ds, boxInf, NULL) < 0)
		return SOAP_ERR;

	switch (update->status) {
	case CMDSTA_QUEUED:
		status = CMDSTA_SEND;
		break;
	case CMDSTA_EXECUTED:
	case CMDSTA_DELETED:
	case CMDSTA_EXECERR:
	case CMDSTA_EXECUTING:
	default:
		status = update->status;
		break;
	}

	if (cid < 0) {
		struct boxCmd *cmd = &update->cmd;
		struct timeval ttime;
		sscanf(cmd->trigtime, "%lu", &ttime.tv_sec);
		ttime.tv_usec = 0;
		*retval = cid;
	} else {
		*retval = 0;
	}

	struct timeval etime;
	sscanf(update->timestamp, "%lu", &etime.tv_sec);

	return SOAP_OK;
}

/**
 * Get file (Updates etc)
 */
int liabdatalogger__fileGet(struct soap *soap, char *filename,
		struct boxInfo *boxInf, struct filetransf *filetf) {
	char filepath[256] = "\0";
	size_t size = 0;
	char *data = NULL;
	char fileid[64];
	sDSWebservice *ds = (sDSWebservice *) soap->user;
	mode_t mode;
	char md5sum[64] = "N/A";

	ds->func = REMACC_BOX_FILEG;

	if (rpserver_box_contact(ds, boxInf, NULL) < 0)
		return SOAP_ERR;

	sprintf(fileid, "<%s@datalogger.liab.dk>", filename);

	filetf->file_mode = mode;

	filetf->checksum = strdup(md5sum);
	filetf->mineid = strdup(fileid);

	soap_set_mime(soap, NULL, NULL); // enable MIME

	if (soap_set_mime_attachment(soap, data, size, SOAP_MIME_BINARY,
			"application/x-gzip", fileid, NULL, NULL)) {
		soap_clr_mime(soap); // don't want fault with attachments
		free(data);
		fprintf(stderr, "error...\n");
		return soap->error;
	}

	if (filepath[0] != '\0')
		if (unlink(filepath))
			fprintf(stderr, "Error deleting \"%s\": %s\n", filepath, strerror(
					errno));

	return SOAP_OK;
}

int liabdatalogger__fileSet(struct soap *soap, struct boxInfo *boxInf,
		struct filetransf *filetf, int* retval) {
	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;

	*retval = 0;

	if (rpserver_box_contact(ds, boxInf, NULL) < 0)
		return SOAP_ERR;

	LOG(LOG_INFO,"Received file (%d) id: %s %p\n", *retval, filetf->mineid, soap->mime.list);

	struct soap_multipart *attachment;

	for (attachment = soap->mime.list; attachment; attachment
			= attachment->next) {
		char md5sum[32];
		const char *description =
				(*attachment).description ? (*attachment).description
						: filetf->mineid;
		const char *filename =
				(*attachment).description ? (*attachment).location
						: filetf->mineid;
		LOG(LOG_INFO,"MIME attachment:\n");
		LOG(LOG_INFO,"Memory=%p\n", (*attachment).ptr);
		LOG(LOG_INFO,"Size=%lu\n", (*attachment).size);
		LOG(LOG_INFO,"Encoding=%d\n", (int)(*attachment).encoding);
		LOG(LOG_INFO,"Type=%s\n", (*attachment).type?(*attachment).type:"null");
		LOG(LOG_INFO,"ID=%s\n", (*attachment).id?(*attachment).id:"null");
		LOG(LOG_INFO,"Location=%s\n", (*attachment).location?(*attachment).location:"null");
		LOG(LOG_INFO, "Description=%s\n", (*attachment).description?(*attachment).description:"null");

		LOG(LOG_INFO, "(test)md5local %s md5remote %s\n", md5sum, filetf->checksum);
	}

	LOG(LOG_INFO, "%s():%d - done serving", __FUNCTION__, __LINE__);

	return SOAP_OK;
}

int liabdatalogger__pair(struct soap *soap, struct boxInfo *boxInf,
		char *localid, int dopair, struct rettext *rettxt) {

	char rettxt_[512];
	int retval = 0;
	sDSWebservice *ds;
	ds = (sDSWebservice *) soap->user;

	ds->func = REMACC_BOX_PAIR;

	memset(rettxt_, 0, sizeof(rettxt_));

	retval = -1;

	if (retval > 0) {
		rettxt->text = strdup(rettxt_);
		rettxt->retval = 1;
	} else if (retval == 0) {
		rettxt->text = strdup("");
		rettxt->retval = 0;
	} else {
		rettxt->text = strdup("");
		rettxt->retval = -1;
	}

	return SOAP_OK;

}
