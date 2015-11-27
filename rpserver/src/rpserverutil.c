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
#include <string.h>
#include <sys/queue.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <assert.h>


#include <soapH.h>	/* include generated proxy and SOAP support */
#include "liabdatalogger.nsmap"
#include "rpserver_db.h"
#include "rpserver_glob.h"
#include "rpserver.h"


struct logunit{
    char *unit;
    int flowcnv;
};


/* struct logunit logunits[]= {			\ */
/*     {"_J", 1},					\ */
/*     {"", -1},					\ */
/*     {"cJ", 1},					\ */
/*     {"_m³", 3600},				\ */
/*     {"e", -1},					\ */
/*     {"V", 0},					\ */
/*     {"MB", 0},					\ */
/*     {"W", 0},					\ */
/*     {"°C", 0},					\ */
/*     {"l/min", 0},				\ */
/*     {"bar", 0},					\ */
/*     {NULL, 0}					\ */
/* }; */

/* int logunits_get_flowcnv(const char *unit) */
/* { */
/*     int i = 0; */

/*     while(logunits[i].unit){ */
/* 	if(strcmp(unit, logunits[i].unit)==0) */
/* 	    return logunits[i].flowcnv; */
/* 	i++; */
/*     } */

/*     return 0; */
/* } */


int system_run(const char *format, ...);
char *dupsprintf(const char *format, ...);


const  char *HelpText =
 "contdaemutil: util for contdaem... \n"
 " -i <iid>         : installation id\n"
 " -c <cmd>         : command name\n"
 " -v <value>       : command parameter/value\n"
 " -t <time>        : command trigger time\n"
 " -u <uid>         : command user\n"
 " -a               : add to list\n"
 " -s               : add to list sequenced with the previous\n"
 " -S <cid>         : add to list sequenced with cid as previous\n"
 " -w               : wait for completion\n"
 " -F <imagefile>   : Do firmware update\n" 
 " -p <file> <dest> : Putfile, <file> is inputfile , <dest> is destination on box\n"
 " -g <file> <dest> : Getfile, (not implemented yet)\n"
 " -o <confname>    : Output config where <confname> is either \"contdaem.conf\" or \"licon.conf\"\n"
 " -h         : this help text\n"
 "Christian Rostgaard, October 2010\n" 
 "";

struct rputil{
    dbi_conn *db;
    int iid;
    int pseq;  
    int user;
    time_t ttimep;
};


/* int rpserver_evtid_get_iid(dbi_conn *db, int evt_id) */
/* { */
     
/*     int iid = -1; */
/*     dbi_result *result;  */

/*     result = dbi_conn_queryf(db,   */
/* 			     "SELECT iid FROM `rembox_etypes_iid` WHERE evt_id=%d", evt_id); */
    

/*     if(result){ */
/* 	while(dbi_result_next_row(result)) { */
/* 	    iid = dbi_result_get_int(result, "iid"); */
/* 	} */
/*     } else { */
/*         const char *err; */
/*         dbi_conn_error(db, &err); */
/*         LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n",  */
/*             __FUNCTION__, __LINE__, err); */
/*         return -2; */
/*     } */

/*     return iid; */

/* } */

enum sum_mode{
    SMODE_LEVEL,
    SMODE_FLOW,
};


/* int rpserver_day_sum_evt(dbi_conn *db, int evt_id , int iid ,  */
/* 		     time_t sum_begin, time_t sum_end, */
/* 			 int flowcnv) */
/* { */
/*     char db_name[32]; */
/*     char db_time_start[64]; */
/*     char db_time_end[64]; */
/*     dbi_result *result = NULL; */
/*     double value = 0; */

/*     sprintf(db_name, "rembox_log"); */

/*     rpserver_db_prep_time(db_time_start, 64, sum_begin); */
/*     rpserver_db_prep_time(db_time_end, 64, sum_end); */

/*     if(iid <0) */
/* 	iid = rpserver_evtid_get_iid(db, evt_id); */
    
/*     if(flowcnv ==0){ */
/* 	result = dbi_conn_queryf(db,   */
/* 				  "SELECT AVG(value) AS value , IFNULL( AVG(value) , 1) AS nul FROM %s" */
/* 				  " WHERE eid=%d AND time >= '%s' AND time < '%s' ", */
/* 				  db_name, evt_id, db_time_start, db_time_end); */

/*     } else if( flowcnv > 0){ */
/* 	result = dbi_conn_queryf(db,   */
/* 				 "SELECT SUM(value) AS value, IFNULL(SUM(value), 1) AS nul FROM %s" */
/* 				  " WHERE eid=%d AND time >= '%s' AND time < '%s' ", */
/* 				 db_name, evt_id, db_time_start, db_time_end); */
/*     } else { */
/* 	return 0; */
/*     }  */

/*     if (result) { */
/* 	if(dbi_result_next_row(result)) { */
/* 	    double nul = dbi_result_get_double(result, "nul"); */
/* 	    value = dbi_result_get_double(result, "value"); */
/* 	    if(nul != value) */
/* 		return 0; */
/* 	} else { */
/* 	    return 0; */
/* 	} */
/*     } else { */
/* 	const char *err; */
/*         dbi_conn_error(db, &err); */
/*         LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n",  */
/*             __FUNCTION__, __LINE__, err); */
/*         return -1; */
/*     } */


/*     result = dbi_conn_queryf(db, "INSERT INTO rembox_day_logs (eid, time, value) " */
/* 			     "VALUES (%d, '%s', %f)" */
/* 			     " ON DUPLICATE KEY UPDATE value=%f " */
/* 			     , evt_id, db_time_start, value */
/* 			     , value); */

/*     if(!result) { */
/* 	const char *err; */
/*         dbi_conn_error(db, &err); */
/*         LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n",  */
/*             __FUNCTION__, __LINE__, err); */
/*         return -1; */
/*     } */


/*     return 0; */
    

    
/* } */

/* int rpserver_day_sum(dbi_conn *db, time_t day) */
/* { */
/*     struct tm time_begin; */
/*     struct tm time_end; */
/*     time_t t_begin; */
/*     time_t t_end; */
/*     dbi_result *result = NULL; */

/*     if(!day){ */
/* 	time(&day); */
/*     } */

/*     localtime_r(&day, &time_begin); */

/*    time_begin.tm_sec = 0;         /\* seconds *\/ */
/*    time_begin.tm_min = 0;         /\* minutes *\/ */
/*    time_begin.tm_hour = 0;        /\* hours *\/ */


/*    memcpy(&time_end,&time_begin, sizeof(time_end)); */

/*    time_end.tm_mday++; */

/*    t_begin = mktime(&time_begin); */
/*    t_end   = mktime(&time_end); */

/*    localtime_r(&t_begin, &time_begin); */
/*    localtime_r(&t_end, &time_end); */

/*    printf("Sum begin: %s", asctime(&time_begin)); */

   

/*    printf("Sum start: %s", asctime(&time_end)); */


/*    result = dbi_conn_queryf(db,   */
/* 			    "SELECT evt_id, iid, unit FROM rembox_etypes_iid" */
/* 			    " WHERE iid IS NOT NULL AND type = '' AND unit != '' AND unit != 'e' "); */

   
/*    if (result) { */
/*        while(dbi_result_next_row(result)) { */
/* 	   int flowcnv = logunits_get_flowcnv(dbi_result_get_string(result, "unit")); */

/* 	   rpserver_day_sum_evt(db, dbi_result_get_int(result, "evt_id"),  */
/* 				dbi_result_get_int(result, "iid"),   */
/* 				t_begin, t_end,  */
/* 				flowcnv);  */
/* 	} */
/*     } else { */
/* 	const char *err; */
/*         dbi_conn_error(db, &err); */
/*         LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n",  */
/*             __FUNCTION__, __LINE__, err); */
/*         return -1; */
/*     } */



 
/* } */



void rputil_init(struct rputil *util)
{
    util->db = NULL ;
    util->iid = -1;
    util->pseq = 0;  
    util->user = 0;
    util->ttimep = time(NULL);
}

struct timeval *time_read(const char *timestr, struct timeval *time)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));

    if(timestr[0]=='+'){
	int offset = atoi(timestr +1);
	gettimeofday(time, NULL);
	time->tv_sec += offset;
	return time;
    } else if(sscanf(timestr, "%u-%u-%u %u:%u:%u",
		     &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		     &tm.tm_hour, &tm.tm_min, &tm.tm_sec)>=3){
	tm.tm_year-=1900;
	tm.tm_mon -=1;

	printf("time %s", asctime(&tm));

	time->tv_sec = mktime(&tm);
	time->tv_usec = 0;
	return time;
    }
		     

    return NULL;

}



int util_txfileprep( const char *file,  const char *tmpfile)
{
    char *tmppath = NULL;
    int retval = 0;
  
    fprintf(stderr, "hej\n");

    /* create txdir if not exists */    
    retval = system_run("mkdir -p \"%s\"",  DEFAULT_FILEPUT_DIR);
    if(retval){
	fprintf(stderr, "Error creating dir %s \n",  DEFAULT_FILEPUT_DIR);
	return -EFAULT;
    }

    chmod(DEFAULT_FILEPUT_DIR, 0777);

    /* set file path */
    tmppath = dupsprintf("%s/%s", DEFAULT_FILEPUT_DIR, tmpfile);

    retval = system_run("cp \"%s\" \"%s\"",  file, tmppath);
    fprintf(stderr, "hej2\n");

    if(retval){
	fprintf(stderr, "Error copying file %s --> %s  (%d)\n", file, tmppath , retval );
	goto out;
    }    

    fprintf(stderr, "hej3\n");

    chmod(tmppath, 0666);

  out:
    free(tmppath);

    return retval;

}


int util_putfile(struct rputil *util, const char *file, const char *dest, const char *mode)
{
    char *cmdname = "getfile";
    char *tmpfile = NULL;
    char param[256];
    int retval = 0;
    int cid = remboxdb_cmd_cid_next( util->db);
    
    if((dest)&&dest[0]=='-'){
	fprintf(stderr, "Unknown destination %s\n", dest);
	return -EFAULT;
    }
    fprintf(stderr, "file 0\n");
    
    if(file[0]=='_'){
	tmpfile = strdup(file);
    } else {
    /* copy file to DEFAULT_FILEGET_DIR */
	tmpfile = dupsprintf("txfile%4.4d-%5.5d", util->iid, cid);
	


	retval = util_txfileprep(file,  tmpfile);
	
	if(retval){
	    fprintf(stderr, "Error preparing file %s \n",  file);
	    goto out;
	} 
    } 

    fprintf(stderr, "file 1\n");

    if((mode)&&(mode[0]!='-')){
	sprintf(param, "%s,%s,%s", tmpfile, dest, mode);
    } else {
	struct stat statbuf;
	if(stat(file, &statbuf)==-1){
	    fprintf(stderr, "could not stat file \"%s\": %s\n", file, strerror(errno));
	    return -errno;
	}
	sprintf(param, "%s,%s,%o", tmpfile, dest, statbuf.st_mode);
    }
    
    fprintf(stderr, "file %p, %d, %s, %s, %d, %d, %d\n" , util->db, util->iid, cmdname, param, 
				  util->ttimep, util->pseq, util->user);

    util->pseq = remboxdb_cmd_add(util->db, util->iid, cmdname, param, 
				  util->ttimep, util->pseq, util->user);
    
    if(util->pseq < 0){
	fprintf(stderr, "Error adding command \"%s\"\n", cmdname);
	retval = -EFAULT;
	goto out;
    }
    
    fprintf(stderr, "file 3\n");
    
  out:
    free(tmpfile);
    return retval;
    
}


int util_fwupd(struct rputil *util, const char *file)
{
    char *cmdname = "jffsupd";
    char *tmpfile = NULL;
    int cid = remboxdb_cmd_cid_next( util->db);
    int retval = 0;
    
    fprintf(stderr, "bajbb  b \n" );

    tmpfile = dupsprintf("txfwimg%4.4d-%5.5d", util->iid, cid);

    retval = util_txfileprep(file,  tmpfile);

    fprintf(stderr, "bajbb  c\n");

    if(retval){
	fprintf(stderr, "Error preparing file %s \n",  file);
	goto out;
    } 

    fprintf(stderr, "bajbb  d util %p db %p %s %s\n", util, util->db, cmdname, tmpfile);
    fprintf(stderr, "file %p, %d, %s, %s, %d, %d, %d\n" , util->db, util->iid, cmdname, tmpfile, 
				  util->ttimep, util->pseq, util->user);

    util->pseq = remboxdb_cmd_add(util->db, util->iid, cmdname, tmpfile, 
				  util->ttimep, util->pseq, util->user);
    
    if(util->pseq < 0){
	fprintf(stderr, "Error adding command \"%s\"\n", cmdname);
	retval = -EFAULT;
	goto out;
    }

  out:
    free(tmpfile);
    return retval;
    

}


int util_print_config(struct rputil *util, const char *name)
{

    char *buf = NULL;
    size_t len = 0;
    int retval = 0;
    
    retval = remboxdb_conf_get(util->db, util->iid, name, &buf, &len);
    
    if(retval<0)
	fprintf(stderr, "unknown config\n");

    printf("%s", buf);

    free(buf);

    return 0;
}
int onetime_conv(sDSWebservice *ds, int iid);

int main(int narg, char *argp[])
{
    int optc;
    int cid_wait = 0;
    struct rputil util;
    rputil_init(&util);
    
    sDSWebservice *ds;
    char *name  = NULL;
    char *param = NULL;
    struct timeval ttime;

    ds = (sDSWebservice *)calloc(1, sizeof(sDSWebservice));
    ds->iid = -1;

    /* Connect to database */
    if(rpserver_db_connect(ds) < 0) {
    	fprintf(stderr, "%s():%d - error connecting to db\n", __FUNCTION__, __LINE__);
    	exit(1);
    }

    fprintf(stderr, "ds %p\n", &ds);
    fprintf(stderr, "db %p\n", ds->db);
    
    util.db = ds->db;

    while ((optc = getopt(narg, argp, "O:D:i:c:v:t:asS:p:g::o:wCF:h")) > 0) {
        switch (optc){ 
	  case 'O':
	    printf("OTC\n");
	    onetime_conv(ds, atoi(optarg));
	    break;
	  /* case 'D':{ */
	  /*     struct timeval sumtime; */
	  /*     struct timeval *sumtimep = &sumtime; */
	  /*     int count = 1; */

	  /*     if(optarg){ */
	  /* 	  sumtimep = time_read(optarg, &sumtime); */
	  /*     }else{ */
	  /* 	  gettimeofday(sumtimep, NULL); */
	  /*     } */

	  /*     if( argp[optind]) */
	  /* 	  if( argp[optind][0]!='-') */
	  /* 	      count = atoi(argp[optind]); */
	  /*     while(count > 0){ */
	  /* 	  count--; */
	  /* 	  rpserver_day_sum(util.db, sumtime.tv_sec-(count*24*60*60)); */
	  /*     } */
	  /*     break; */
	  /* } */
	  case 'i':
	    util.iid = atoi(optarg);
	    break;
          case 'c': // list db events 
	    name = strdup(optarg);
            break;
	  case 'v':
	    param = strdup(optarg);
	    break;
	  case 't':{
	      struct timeval *tvp;
	      tvp = time_read(optarg, &ttime);
	      if(!util.ttimep){
		  fprintf(stderr, "Error reading time string \"%s\"\n", optarg);
		  exit(0);
	      }
	      util.ttimep = tvp->tv_sec;
	  } break;
	  case 'u':
	    util.user = atoi(optarg);
	  case 'a':
	  case 's':
	  case 'S':
  	    if(optc=='a')
		util.pseq = 0;
	    else if (optc=='S')
		util.pseq = atoi(optarg);
   	    fprintf(stderr, "adding comand to iid %d: %s, %s\n", util.iid, name, param);
	    util.pseq = remboxdb_cmd_add(util.db, util.iid, name, param, 
					 util.ttimep, util.pseq, util.user);
	    if(util.pseq < 0){
		fprintf(stderr, "Error adding command \"%s\"\n", name);
		exit(0);
	    }
	    free(name);
	    free(param);
	    name = NULL;
	    param = NULL;
//	    util.ttimep = NULL;		
	    break;
	  case 'p': // Put file
	    fprintf(stderr, "Put file %s next %s\n", optarg,  argp[optind]);
	    util_putfile(&util, optarg, argp[optind], NULL);
	    break;
	  case 'g':
	    fprintf(stderr, "Get file is not implemenetd\n");
	    break;
	  case 'o':
	    util_print_config(&util, optarg);
	    break;	    
	  case 'w':
	    cid_wait = util.pseq;
	    break;
	  case 'C':
	    util_putfile(&util, "_contdaem.conf", "/jffs2/contdaem.conf", "666");
	    util_putfile(&util, "_licon.conf", "/jffs2/licon.conf", "666");
	    break;
	  case 'F':
	    util_fwupd(&util, optarg);
	    break;
	  case ':':
	    fprintf(stderr, "Missing option argument %c\n", optopt);
	    break;
	  case '?':
  	    fprintf(stderr, "Unknown option %c\n", optopt);
	  case 'h': // Help
            default:
	    fprintf(stderr, "%s", HelpText);
	    exit(0);
	    break;
        }
    }


    if(cid_wait){
	int prev_status = 0;
	int run = 1;
	printf("waiting for cid %d\n",  cid_wait);
	while(run){
	    enum boxcmdsta status = remboxdb_cmd_state(util.db, cid_wait);
	    sleep(1);
	    if(status != prev_status)
		printf("status --> %d\n", status);
	    switch(status){
	      case BOXCMDSTA_QUEUED:
	      case BOXCMDSTA_SEND:
	      case BOXCMDSTA_DELETIG:
	      case BOXCMDSTA_EXECUTING:
		break;
	      case BOXCMDSTA_EXECUTED:
	      case BOXCMDSTA_DELETED:
	      case BOXCMDSTA_EXECDEL:
	      case BOXCMDSTA_EXECERR:
	      case BOXCMDSTA_ERROBS:
	      case BOXCMDSTA_UNKNOWN:
	      default:
	        run = 0;
	      break;
	    }

	    prev_status = status;
	}
	
    }
    
    



    rpserver_db_disconnect(ds);
    free(ds);

    return 0;
    
     

}



char *dupsprintf(const char *format, ...)
{
    char tmpbuf[256];
    va_list ap;

    va_start(ap, format);
    vsnprintf(tmpbuf, sizeof(tmpbuf), format, ap);
    va_end(ap);

    return strdup(tmpbuf);

}


int system_run(const char *format, ...)
{
    int status;
    char cmd[256];
    va_list ap;

    va_start(ap, format);
    vsnprintf(cmd, sizeof(cmd), format, ap);
    va_end(ap);

    fprintf(stderr, "executing: %s\n", cmd);

    status = system(cmd); 
    if(status == -1){
        fprintf(stderr, "Error running \"%s\"\n", cmd);
        return -EFAULT;
    } 
    
    return WEXITSTATUS(status);
}



int rpserverutil_evtid_remove_old(sDSWebservice *ds,int iid, int evt_id)
{
    dbi_result *result; 
    dbi_conn *db = ds->db;

    result = dbi_conn_queryf(db,  
			     "UPDATE `rembox_etypes` SET eid=NULL WHERE evt_id=%d ", evt_id);

    printf("UPDATE `rembox_etypes` SET eid=NULL WHERE evt_id=%d \n", evt_id);

    if(!result){
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return 0;
    }
    
    return 0;
    
}



int rpserverutil_evtid_rename(sDSWebservice *ds,int iid, int prev, int new)
{

    dbi_result *result; 
    dbi_conn *db = ds->db;


    result = dbi_conn_queryf(db,  
			    "UPDATE `rembox_log` SET eid=%d WHERE eid=%d ", new, prev);  
    printf("UPDATE `rembox_log` SET eid=%d WHERE eid=%d \n",  new, prev);
    





    if(!result){
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return 0;
    }
    
    return 0;

}


int onetime_conv(sDSWebservice *ds, int iid)
{
    
      int len = 0;
    dbi_result *result; 
    dbi_conn *db = ds->db;

    result = dbi_conn_queryf(db,  
			     "SELECT * FROM `rembox_etypes_iid` WHERE eid IS NOT NULL AND iid=%d", iid);

    printf("SELECT * FROM `rembox_etypes_iid` WHERE eid IS NOT NULL AND iid=%d\n", iid);


    if(result){
	while(dbi_result_next_row(result)) {
	    int old_eid = dbi_result_get_int(result, "eid");
	    int evt_id = dbi_result_get_int(result, "evt_id");
	    printf("iid:%d (%d-->%d), %s\n", dbi_result_get_int(result, "iid"), old_eid, evt_id , dbi_result_get_string(result, "ename"));
	    //if(old_eid==1){
		rpserverutil_evtid_remove_old(ds, iid, evt_id);
		rpserverutil_evtid_rename(ds,iid, old_eid, evt_id);
		//}
	}
    } else {
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return 0;
    }

    return 0;

}
