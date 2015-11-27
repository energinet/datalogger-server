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

#define USE_DBI

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <dbi/dbi.h>
#include <stdsoap2.h>

#include <soapH.h>	/* include generated proxy and SOAP support */
#include "liabdatalogger.nsmap"
#include "rpserver.h"

#include "rpserver_db.h"
#include "rpserver_glob.h"
#include "rpfileutil.h"
#include <remboxdb.h>

enum cmdsta rpserver_cmd_fromremdb(enum boxcmdsta status);


enum boxcmdsta rpserver_cmd_toremdb(enum cmdsta status);


int main(int argc, char *argv[])
{
  struct soap soap;
  sDSWebservice *ds;
  SOAP_SOCKET m, s; /* master and slave sockets */
  int ret = 0;

  openlog("rpserver", LOG_PID, LOG_USER);
  
  setlogmask(LOG_UPTO(LOG_WARNING));
  //setlogmask(LOG_UPTO(LOG_DEBUG));

  LOG(LOG_INFO, "%s():%d - start (last change %s)\n", __FUNCTION__, __LINE__, __TIMESTAMP__);  
  
  ds = (sDSWebservice *)calloc(1, sizeof(sDSWebservice));

  //  ds->iid = -1;
  ds->boxid = -1;
  ds->acclev = REMACC_BOXLEV_NONE;


  /* Connect to database */
  if(rpserver_db_connect(ds) < 0) {
    LOG(LOG_ERR, "%s():%d - error connecting to db", __FUNCTION__, __LINE__);
    return SOAP_FATAL_ERROR;
  }

  soap_init1(&soap, SOAP_C_UTFSTRING | SOAP_IO_STORE );

  /* Assign user data */
  soap.user = (void *)ds; 

  if (argc < 2) {
      	  /****************************************************/
	  /* CGI                                              */
	  /****************************************************/
         if(remboxdb_access_box_check(ds->db, REMACC_TYPE_HTACCESS, getenv("REMOTE_USER"),
				      NULL, &ds->acclev, &ds->boxid) > REMACC_BOXLEV_NONE){
	   ds->atid = REMACC_TYPE_HTACCESS;
	 }
	  
	  LOG(LOG_DEBUG, "%s():%d - called as CGI", __FUNCTION__, __LINE__);
	  ret = soap_serve(&soap);	/* serve as CGI application */
	  if(ret != 0) {
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
    for ( ; ; ) {
      s = soap_accept(&soap);
      fprintf(stderr, "Socket connection successful: slave socket = %d\n", s);
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

  snprintf(scriptaddr, sizeof(scriptaddr), "http%s://%s%s" ,(atoi(getenv("SERVER_PORT"))==443)?"s":""
		   , getenv("SERVER_NAME"), getenv("SCRIPT_NAME"));


  remboxdb_access_log_box(ds->db, getenv("REMOTE_ADDR"), scriptaddr, ds->func, ds->boxid, ds->atid, ds->count);


  out:
  rpserver_db_disconnect(ds);
  free(ds);

  LOG(LOG_INFO, "%s():%d - done serving...", __FUNCTION__, __LINE__);

  closelog();

  return ret;
}

int check_basic_auth(struct soap *soap)
{
  sDSWebservice *ds;
  ds = (sDSWebservice *)soap->user;

  if (!soap->userid || !soap->passwd) {
    printf("Setting authrealm\n");
    soap->authrealm = "Mikkels realm";
    return 401;
  } else {
    return rpserver_db_authenticate(ds->db, soap->userid, soap->passwd);
  }
}

/******************** External API ********************/
int liabdatalogger__getApiVersion(struct soap *soap, struct apiVersion *versionResp)
{
  sDSWebservice *ds;
  ds = (sDSWebservice *)soap->user;
  
  /* Validate user */
  if(rpserver_db_authenticate(ds->db, soap->userid, soap->passwd) < 0) {
    LOG(LOG_WARNING, "%s():%d - user authentication failed (%s:%s)", 
           __FUNCTION__, __LINE__, soap->userid, soap->passwd);
    return 401;
  }

  LOG(LOG_DEBUG, "%s():%d - start", __FUNCTION__, __LINE__);

  versionResp->description = strdup("LIAB Datalogger WebService");
  versionResp->major = 0;
  versionResp->minor = 0;
  versionResp->sub = 1;
  
  LOG(LOG_DEBUG, "%s():%d - end", __FUNCTION__, __LINE__);
  return SOAP_OK;
}

int liabdatalogger__statusTelegram(struct soap *soap,
                                   struct status *status,
                                   struct signalResponse *out)
{
//  dbi_result result;
//  int radid;
  int retval = SOAP_ERR;
  
  sDSWebservice *ds;
  ds = (sDSWebservice *)soap->user;

  /* Validate user */
  if(rpserver_db_authenticate(ds->db, soap->userid, soap->passwd) < 0) {
    LOG(LOG_WARNING, "%s():%d - user authentication failed (%s:%s)", 
           __FUNCTION__, __LINE__, soap->userid, soap->passwd);
    return 401;
  }

  /* Validate user */
  LOG(LOG_DEBUG, "%s():%d - start", __FUNCTION__, __LINE__);

  LOG(LOG_DEBUG, "%s():%d - statusType    : %s", __FUNCTION__, __LINE__, status->type);
  LOG(LOG_DEBUG, "%s():%d - statusName    : %s", __FUNCTION__, __LINE__, status->name);
  LOG(LOG_DEBUG, "%s():%d - statusContent : %s", __FUNCTION__, __LINE__, status->value);

  retval = SOAP_OK;
  
  LOG(LOG_DEBUG, "%s():%d - end", __FUNCTION__, __LINE__);
  return retval;
}

void boxCmdSet(struct boxCmd *cmd, const char *id, int sequence, int pseq, 
	       unsigned long trigger_time, int status, int param_cnt, ...)
{
    
    va_list ap;
    int i;

    LOG(LOG_DEBUG, "%s():%d %s %d %d\n", __FUNCTION__, __LINE__, id, sequence, pseq );

    cmd->name      = strdup(id);
    cmd->sequence  = sequence;
    cmd->pseq      = pseq;
    cmd->params    = malloc(sizeof(void*)*param_cnt);
    cmd->paramsCnt = param_cnt;
    cmd->status    = status;
    {
        char ttime[32];
        sprintf(ttime, "%ld",  trigger_time);
        cmd->trigtime = strdup(ttime);
    }
    va_start (ap, param_cnt);
    for(i=0;i<param_cnt;i++){
        cmd->params[i] = strdup(va_arg(ap,const char *));
    }
    va_end(ap);
}


void boxCmdFill(struct boxCmd *cmd, int maxcnt, struct rembox_cmd* list)
{

    if(list==NULL || maxcnt < 0){
	return;
    }

    boxCmdSet(cmd, list->cmd, list->cmd_id, list->pseq,  
	      list->ttime, list->status, 1, list->param);

    boxCmdFill(cmd+1, maxcnt-1, list->next);
}

void fillHostRet(dbi_conn *db, int device_id, struct hostReturn *hostRet)
{
    int  cmd_cnt, i;

    if(!db){
        hostRet->cmds = NULL;
        hostRet->cmdCnt = 0;
        return;
    }
    
    if(device_id<0){
        hostRet->cmds = NULL;
        hostRet->cmdCnt = 0;        
        return;
    }

    struct rembox_cmd* cmdlist = remboxdb_cmd_xmitist(db, device_id, 100, &cmd_cnt);

    hostRet->cmds = malloc(sizeof(struct boxCmd)*cmd_cnt);
    hostRet->cmdCnt = cmd_cnt;
    
    boxCmdFill(hostRet->cmds, cmd_cnt, cmdlist);

    rembox_cmd_delete(cmdlist);

    for(i = 0 ; i < hostRet->cmdCnt ; i++){ 
	struct boxCmd *cmd = hostRet->cmds + i; 
	LOG(LOG_DEBUG, "%s():%d - cmd: %d \"%s\"  %p\n", __FUNCTION__, __LINE__, i, cmd->name, cmd); 
	
    } 

    if(hostRet->cmdCnt == 0)
	LOG(LOG_DEBUG, "%s():%d - cmd count == 0\n", __FUNCTION__, __LINE__);

    return;
}

int rpserver_box_contact(sDSWebservice *ds, struct boxInfo *boxInf, struct hostReturn *hostRet)
{
    char externalip[32];
    char scriptaddr[256];
    sprintf(externalip, "%s", getenv("REMOTE_ADDR"));
    snprintf(scriptaddr, sizeof(scriptaddr), "http://%s%s" , getenv("SERVER_NAME"), getenv("SCRIPT_NAME"));
    
    if(ds->acclev == REMACC_BOXLEV_FULL){
	LOG(LOG_DEBUG, "before box check == REMACC_BOXLEV_FULL");
    } else if(REMACC_BOXLEV_NONE != remboxdb_access_box_check(ds->db, REMACC_TYPE_MAC,
						       boxInf->name, NULL, &ds->acclev, &ds->boxid)){
	LOG(LOG_DEBUG, "after box check != NONE");
	ds->atid = REMACC_TYPE_MAC;
    } else {
	LOG(LOG_DEBUG, "after box check == NONE");
	ds->boxid =  remboxdb_dev_boxid_get(ds->db, boxInf->name);
	ds->acclev = (remboxdb_access_old_enabled(ds->db)==1)?REMACC_BOXLEV_FULL:REMACC_BOXLEV_LIMITED;
	ds->atid = REMACC_TYPE_OLD;
    }

    if(ds->boxid <= 0)
	return REMACC_BOXLEV_NONE;

    remboxdb_dev_update_id(ds->db, ds->boxid , boxInf->conn.ip, boxInf->status.uptime, externalip, 
			   boxInf->rpversion, boxInf->localid ); 

    LOG(LOG_DEBUG, "after box check boxid %d, acclev %d, atid %d", ds->boxid, ds->acclev, ds->atid);
    if(ds->boxid<0){
	LOG(LOG_DEBUG, "%s():%d - error getting device_id for '%s' : %d \n", 
            __FUNCTION__, __LINE__, boxInf->name, ds->boxid);
	return -1;
    }

    if(hostRet)
	fillHostRet(ds->db, ds->boxid,hostRet);
    
    return ds->acclev;
}


int liabdatalogger__sendBoxInfo(struct soap *soap, struct boxInfo *boxInf, struct hostReturn *hostRet){

    int i;
    sDSWebservice *ds;
    ds = (sDSWebservice *)soap->user;
    struct timeval tv;

    ds->func = REMACC_BOX_INFO;

//    fprintf(stderr, "box type: %s , name %s\n", boxInf->type, boxInf->name);
    LOG(LOG_DEBUG, "box type: %s , name %s \n", boxInf->type, boxInf->name);
    fillHostRet(NULL, -1,  hostRet);
    
    for(i = 0 ; i < hostRet->cmdCnt ; i++){ 
	struct boxCmd *cmd = hostRet->cmds + i; 
	LOG(LOG_DEBUG, "cmd: %d \"%s\"  %p\n",  i, cmd->name, cmd); 
    } 
    
    gettimeofday(&tv,NULL);

    return SOAP_OK;
}



int liabdatalogger__sendMeasurments(struct soap *soap, struct measurments* meas, struct hostReturn *hostRet) 
{ 
    sDSWebservice *ds = (sDSWebservice *)soap->user;
    struct boxInfo *boxInf = &meas->boxInf;
    int i, n, count = 0 ; 

    ds->func = REMACC_BOX_MEAS;

    if(rpserver_box_contact(ds, boxInf, hostRet)<REMACC_BOXLEV_FULL){
      LOG(LOG_WARNING, "discarded data for %d\n", ds->boxid);

	return SOAP_OK;
    }

    for(i = 0; i < meas->seriesCnt ; i++){
        struct eventSeries *eseries =  meas->series + i;
	int evt_id = remboxdb_log_evtid_get(ds->db,  ds->boxid, eseries->ename);
	struct timeval tv_end;
        for(n = 0; n < eseries->measCnt ; n++){
            struct eventItem *eitem = eseries->meas +n;
	    struct timeval tv;
	    remboxdb_time_fromstr(&tv, eitem->time);
	    remboxdb_log_add(ds->db, evt_id, &tv , eitem->value);
	    count++;
        }

	remboxdb_time_fromstr(&tv_end, eseries->time_end);
	remboxdb_log_evtid_timupd(ds->db, evt_id, &tv_end);
    }

    ds->count = count;

    return SOAP_OK;
    
}

int liabdatalogger__sendMetadata(struct soap *soap,struct measMeta* metaData,  struct hostReturn *hostRet)
{
    int i, count = 0 ;
    sDSWebservice *ds = (sDSWebservice *)soap->user;
    struct boxInfo *boxInf = &metaData->boxInf;
 
    ds->func = REMACC_BOX_META;

    if(rpserver_box_contact(ds, boxInf, hostRet)<REMACC_BOXLEV_LIMITED)
	return SOAP_OK; //Ignore data

    for(i = 0; i < metaData->etypeCnt; i++){
        struct eTypeMeta *etype = metaData->etypes +i;
	remboxdb_log_evtid_update(ds->db,  ds->boxid, etype->ename, etype->hname, etype->unit, etype->type);
	count++;
    }
    
    ds->count = count;
    
    return SOAP_OK;
}




/**
 * Send command status update 
 * @param update, command that has been update
 * @param retval it the cid in the rembox database
 */
int liabdatalogger__cmdUpdate(struct soap *soap, struct boxCmdUpdate *update, int *retval)
{
    int status;
    sDSWebservice *ds = (sDSWebservice *)soap->user;
    int cid = update->sequence;
    struct boxInfo *boxInf = &update->boxInf;

    ds->func = REMACC_BOX_CMDUP;

    if(rpserver_box_contact(ds, boxInf, NULL)<REMACC_BOXLEV_LIMITED)
	return SOAP_ERR;

    switch(update->status)
    {
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

    if(cid<0){
	struct boxCmd *cmd = &update->cmd;
	struct timeval ttime;
	sscanf(cmd->trigtime, "%lu", &ttime.tv_sec);
	ttime.tv_usec = 0;
	cid = remboxdb_cmd_add(ds->db, ds->boxid, cmd->name, cmd->params[0],  
			       ttime.tv_sec, cmd->pseq, CMDUSER_BOX);

	*retval = cid;
    } else {
	*retval = 0;
    }

    struct timeval etime;
    sscanf(update->timestamp, "%lu", &etime.tv_sec);
    
    if(remboxdb_cmd_set_status(ds->db, cid, ds->boxid, rpserver_cmd_toremdb(status),  
			       etime.tv_sec,  update->retval, BOXCMDSTA_NONE)<0)
	return SOAP_ERR;
    
    return SOAP_OK;
}


int liabdatalogger__fileGet(struct soap *soap, char *filename, struct boxInfo *boxInf, struct filetransf *filetf)
{
    char filepath[256] = "\0";
    size_t size  = 0;
    char *data= NULL;
    char fileid[64];
    sDSWebservice *ds = (sDSWebservice *)soap->user;
    mode_t mode;
    char md5sum[64]= "N/A";
    
    ds->func = REMACC_BOX_FILEG;

    if(rpserver_box_contact(ds, boxInf, NULL)<REMACC_BOXLEV_LIMITED)
	return SOAP_ERR;

    sprintf(fileid, "<%s@datalogger.liab.dk>", filename);

    if(filename[0]=='_'){
	/* config file */
	mode = 0666; 
	if(remboxdb_conf_get(ds->db, ds->boxid, filename+1, &data, &size)<0)
	    return SOAP_ERR; 
    } else if(filename[0]=='@'){
	 mode = 0666;
	 if(remboxdb_file_read(ds->db, filename+1, &data, &size)<=0)
	      return SOAP_ERR;
    } else {
	/* fil system file */
	sprintf(filepath, "%s/%s", DEFAULT_FILEPUT_DIR, filename);

	if(rpfile_read(filepath, &data , &size, &mode)<=0){ 
	    unlink(filepath);
	    return SOAP_ERR; 
	} 
    }

    filetf->file_mode = mode; 
    rpfile_md5((unsigned char*)data, size, md5sum); 
    filetf->checksum  = strdup(md5sum);
    filetf->mineid    = strdup(fileid); 
    
    //    fprintf(stderr, "..mode %lo, id %s, chk %s\n", filetf->file_mode, filetf->mineid , filetf->checksum); 
    //fprintf(stderr, "soap_set_mime(%p, NULL, NULL);\n", soap);    
    soap_set_mime(soap, NULL, NULL); // enable MIME 
    
    //    fprintf(stderr, "attaching...\n");    
    
    if (soap_set_mime_attachment(soap, data, size, SOAP_MIME_BINARY, "application/x-gzip", fileid, NULL, NULL)) {
	soap_clr_mime(soap); // don't want fault with attachments
	free(data);
	fprintf(stderr, "error...\n");    
	return soap->error;
    }
    // fprintf(stderr, "done...\n");    

    if(filepath[0] != '\0')
      if(unlink(filepath))
	fprintf(stderr, "Error deleting \"%s\": %s\n",filepath, strerror(errno)); 

    return SOAP_OK;
} 

int liabdatalogger__fileSet(struct soap *soap, struct boxInfo *boxInf, struct filetransf *filetf,  int* retval)
{
	sDSWebservice *ds;
    ds = (sDSWebservice *)soap->user;

	*retval = 0;


	if(rpserver_box_contact(ds, boxInf, NULL)<REMACC_BOXLEV_LIMITED)
		return SOAP_ERR;
	
	LOG(LOG_INFO,"Received file (%d) id: %s %p\n", *retval, filetf->mineid, soap->mime.list);

	struct soap_multipart *attachment;
		
	for (attachment = soap->mime.list; attachment; attachment = attachment->next){
		char md5sum[32];
		const char *description = (*attachment).description?(*attachment).description:filetf->mineid;
		const char *filename = (*attachment).description?(*attachment).location:filetf->mineid;
		LOG(LOG_INFO,"MIME attachment:\n");
		LOG(LOG_INFO,"Memory=%p\n", (*attachment).ptr);
		LOG(LOG_INFO,"Size=%lu\n", (*attachment).size);
		LOG(LOG_INFO,"Encoding=%d\n", (int)(*attachment).encoding);
		LOG(LOG_INFO,"Type=%s\n", (*attachment).type?(*attachment).type:"null");
		LOG(LOG_INFO,"ID=%s\n", (*attachment).id?(*attachment).id:"null");
		LOG(LOG_INFO,"Location=%s\n", (*attachment).location?(*attachment).location:"null");
		LOG(LOG_INFO, "Description=%s\n", (*attachment).description?(*attachment).description:"null");

		rpfile_md5( (unsigned char *)attachment->ptr, attachment->size, md5sum);
				
		LOG(LOG_INFO, "(test)md5local %s md5remote %s\n", md5sum, filetf->checksum);
				
				
		if(rpfile_md5chk((unsigned char *)attachment->ptr, attachment->size, filetf->checksum)!=0){
			LOG(LOG_ERR, "sum FAULT\n");
		} else {
			LOG(LOG_ERR, "sum OK\n");
			remboxdb_file_create(ds->db,ds->boxid, filename, description, attachment->ptr, 
					     attachment->size, filetf->checksum);
		}


	}

	LOG(LOG_INFO, "%s():%d - done serving", __FUNCTION__, __LINE__);

	return SOAP_OK;
}

int liabdatalogger__pair(struct soap *soap, struct boxInfo *boxInf, char *localid, int dopair, struct rettext *rettxt)
{
    
    char rettxt_[512]; 
    int retval = 0;
    sDSWebservice *ds;
    ds = (sDSWebservice *)soap->user;

    ds->func = REMACC_BOX_PAIR;

    memset(rettxt_, 0, sizeof(rettxt_));

//    fprintf(stderr, "liabdatalogger__pair localid \"%s\" dopair %d\n",localid, dopair); 
    
    if(rpserver_db_pair_idcheck(localid)==0)
	if(dopair){
	    retval = rpserver_db_pair_exec(ds->db, localid, boxInf->name, rettxt_, sizeof(rettxt_));
	} else {
	    retval = rpserver_db_pair_lookup(ds->db, localid,  rettxt_, sizeof(rettxt_));
	}
    else
	retval = -1;
    
//    fprintf(stderr, "liabdatalogger__pair text \"%s\" retval %d\n",rettxt_, retval); 

    if(retval > 0){
	rettxt->text = strdup(rettxt_);
	rettxt->retval = 1;
    } else if(retval == 0) {
	rettxt->text = strdup("");
	rettxt->retval = 0;
    } else {
	rettxt->text = strdup("");
	rettxt->retval = -1;
    }

    

    return SOAP_OK;
    
}




enum cmdsta rpserver_cmd_fromremdb(enum boxcmdsta status)
{
    switch(status){
      case  BOXCMDSTA_QUEUED:
	return CMDSTA_QUEUED;
      case  BOXCMDSTA_SEND:
	return CMDSTA_SEND;
      case  BOXCMDSTA_EXECUTING:
	return CMDSTA_EXECUTING;
      case  BOXCMDSTA_EXECUTED:
	return CMDSTA_EXECUTED;
      case  BOXCMDSTA_DELETIG:
	return CMDSTA_DELETIG;
      case  BOXCMDSTA_DELETED:
	return CMDSTA_DELETED;
      case  BOXCMDSTA_EXECDEL:
	return CMDSTA_EXECDEL;
      case   BOXCMDSTA_EXECERR:
	return CMDSTA_EXECERR;
      case BOXCMDSTA_ERROBS:
	return CMDSTA_ERROBS;
      default:
	return CMDSTA_UNKNOWN;
    }
}

enum boxcmdsta rpserver_cmd_toremdb(enum cmdsta status)
{
    switch(status){
      case CMDSTA_QUEUED:
	return BOXCMDSTA_QUEUED;
      case CMDSTA_SEND:
	return BOXCMDSTA_SEND;
      case CMDSTA_EXECUTING:
	return BOXCMDSTA_EXECUTING;
      case CMDSTA_EXECUTED:
	return BOXCMDSTA_EXECUTED;
      case CMDSTA_DELETIG:
	return BOXCMDSTA_DELETIG;
      case CMDSTA_DELETED:
	return BOXCMDSTA_DELETED;
      case CMDSTA_EXECDEL:
	return BOXCMDSTA_EXECDEL;
      case CMDSTA_EXECERR:
	return BOXCMDSTA_EXECERR;
      case CMDSTA_ERROBS:
	return BOXCMDSTA_ERROBS;
      default:
	return BOXCMDSTA_UNKNOWN;
    }
}

