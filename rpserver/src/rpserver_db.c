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
#include "rpserver_glob.h"
#include "rpserver_db.h"

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
#include <assert.h>
#include <ctype.h>

//#include <remboxdb.h>


#define DB_TABLE_DEVICES "devices"


int is_text(char *str);

/**********DB CONNECT/DISCONNECT *****************/

/**
 * Open connection to database 
 */
int rpserver_db_connect(sDSWebservice *ds)
{
    const char *err;
    struct dbparams params;
    
    remboxdb_dpparams_fill(&params);


    LOG(LOG_INFO,  "db %s@%s\n", params.db, params.host);

    if(remboxdb_connect(ds, params.host, params.db, 
			    params.user, params.pass) < 0) { 
		LOG(LOG_ERR, "%s():%d - dbi error connecting to database %s@%s\n",  
	    __FUNCTION__, __LINE__, params.db, params.host); 
	return (-1); 
    } 

  LOG(LOG_INFO, "%s():%d - Database "DB_NAME" open\n", __FUNCTION__, __LINE__);

  return 0;
}

/**
 * Close connection to database 
 */
void rpserver_db_disconnect(sDSWebservice *ds)
{
    //    return; //ToDo ?????

    LOG(LOG_INFO, "%s():%d - db closed", __FUNCTION__, __LINE__);

    remboxdb_disconnect(ds);
//    dbi_conn_close(ds->db);
//    dbi_shutdown();

    return;
}


enum eAuthVal{AUTH_UNKNOWN_USER = -3, AUTH_WRONG_PASSWD, AUTH_INACTIVE, AUTH_VALID};

int rpserver_db_authenticate(dbi_conn db, const char *user, const char *password)
{
  int retval = AUTH_UNKNOWN_USER;
  dbi_result result;

  LOG(LOG_INFO, "%s():%d - username: %s  password: %s", __FUNCTION__, __LINE__, user, password);

  return 0;

  result = dbi_conn_queryf(db, 
                           "SELECT * FROM `users` WHERE `username` = '%s' AND `password` = MD5('%s')",
                           user, password);

  LOG(LOG_INFO, "%s():%d - SELECT * FROM `users` WHERE `username` = '%s' AND `password` = '%s'", 
         __FUNCTION__, __LINE__, user, password);

  if (result) {
    if( dbi_result_get_numrows(result) == 1 ) {
      while (dbi_result_next_row(result)) {
          LOG(LOG_INFO, "%s():%d - User \"%s\" is known and ACTIVE.",
                 __FUNCTION__, __LINE__, user);
          retval = AUTH_VALID;
      } 
    } else {
      LOG(LOG_INFO, "%s():%d - User \"%s\" is unknown",
             __FUNCTION__, __LINE__, user);
    }
  }
  dbi_result_free(result);

  return retval;
}



/**********DB UTILS *****************/


/* /\** */
/*  * Prepare db time format from time_t */
/*  *\/ */
/* int rpserver_db_prep_time(char *strtime, int len,  time_t time) */
/* { */
/*     struct tm gmt; */

/*     if(!gmtime_r(&time, &gmt)){ */
/*         fprintf(stderr, "Datetime error\n"); */
/*         return -1; */
/*     } */

/*     return sprintf(strtime, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",  */
/* 		   gmt.tm_year+1900, gmt.tm_mon+1, gmt.tm_mday, */
/* 		   gmt.tm_hour, gmt.tm_min,  gmt.tm_sec); */
    
/* } */

/* /\** */
/*  * Prepare db time format from struct timeval */
/*  *\/ */
/* int rpserver_db_prep_time_tv(char *strtime, int len, struct timeval *time) */
/* { */
/*     return rpserver_db_prep_time(strtime, len,  time->tv_sec); */
/* } */

/* /\** */
/*  * Prepare db time format from string  */
/*  *\/ */
/* int rpserver_db_prep_time_str(char *strtime, int len, const char *ptime) */
/* { */
    
/*     time_t time = 0; */

/*     sscanf(ptime, "%ld", &time); */

/*     return rpserver_db_prep_time(strtime, len,  time); */

/* } */





/*--- PAIR FUNCTIONS ---*/



int rpserver_db_pair_idcheck(char *buffer)
{

    int i = 0;
    int liid = 0;
    char name[512];

    while(buffer[i]){
	buffer[i] = tolower(buffer[i]);
	i++;
    }
  

    if(sscanf(buffer, "%[a-zA-Z]-%d",name, &liid )==2){
	return 0;
    }


    return -1;
}




int rpserver_db_pair_lookup(dbi_conn *db, const char *localid, char *buf, size_t maxlen)
{
    
    dbi_result *result; 
    int len = 0;
        
    result = dbi_conn_queryf(db,  
			     "SELECT hname FROM "DB_TABLE_DEVICES
			     " WHERE localid='%s' AND name IS NULL "
			     ,localid);

    if(result){
	if(dbi_result_next_row(result)) {
	    len +=  snprintf(buf+len, maxlen-len, "%s",
			     dbi_result_get_string(result, "hname"));
	} 
    } else {
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return -1;
    }

    dbi_result_free(result);

    return len;

    
}


int rpserver_db_pair_exec(dbi_conn *db, const char *localid, const char *name, char *buf, size_t maxlen)
{
    dbi_result *result; 
    int len = 0;
    int boxid = -1;

    result = dbi_conn_queryf(db,  "DELETE FROM "DB_TABLE_DEVICES" WHERE name='%s' AND iid IS NULL\n", name);

    if(result){
	dbi_result_free(result);
    } else {
	const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", __FUNCTION__, __LINE__, err);
        return -1;
    }

    result = dbi_conn_queryf(db,  "SELECT device_id FROM "DB_TABLE_DEVICES" WHERE localid='%s' AND name IS NULL\n", localid);
    
    if(result){
	if(dbi_result_next_row(result)) {
	    boxid =  dbi_result_get_int(result, "device_id");
	    LOG(LOG_INFO, "%s():%d - boxid is %d\n", __FUNCTION__, __LINE__, boxid);
	}
	dbi_result_free(result);
    } else {
	const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", __FUNCTION__, __LINE__, err);
        return -1;
    }
    
    if(boxid < 0){
	LOG(LOG_INFO, "%s():%d - boxid %d !!!!\n", __FUNCTION__, __LINE__, boxid);
	return -1;
    }
    
    result = dbi_conn_queryf(db, "UPDATE "DB_TABLE_DEVICES" SET name='%s' WHERE device_id=%d", name, boxid);
    

    if(result){
	dbi_result_free(result);
    } else {
	const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", __FUNCTION__, __LINE__, err);
        return -1;
    } 

    LOG(LOG_INFO, "%s():%d - name updated to '%s' for moxid  %d !!!!\n", __FUNCTION__, __LINE__, name, boxid);

    
    result = dbi_conn_queryf(db, "SELECT hname, localid FROM "DB_TABLE_DEVICES" WHERE  device_id=%d", boxid);
    
    if(result){
	if(dbi_result_next_row(result)) {
	    len +=  snprintf(buf+len, maxlen-len, "%s: %s",
			     dbi_result_get_string(result, "localid"),
			     dbi_result_get_string(result, "hname")
			     );
	}
	dbi_result_free(result);
    } else {
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return -1;
    }

    LOG(LOG_INFO, "%s():%d - read localiid and hname:  %s !!!!\n", __FUNCTION__, __LINE__, name, buf);
    
    int other_pwid = -1;
    int working_pwid = 0;

    result = dbi_conn_queryf(db,  
			     "SELECT pwid , used, atid  FROM acc_devices_pwl JOIN acc_types USING(atid)"
			     " WHERE agid = 1 AND acc_devices_pwl.enable = 1 AND acc_types.enable =1"
			     " AND id = %d  ORDER BY atid ASC", boxid);

    if(result){
	if(dbi_result_next_row(result)) {
	    other_pwid = dbi_result_get_int(result, "pwid");
	    if(dbi_result_get_int(result, "used")==1)
		working_pwid++;
	    if(dbi_result_get_int(result, "atid")== REMACC_TYPE_MAC)
		working_pwid++;
	}
	dbi_result_free(result);
    } else {
        const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", 
            __FUNCTION__, __LINE__, err);
        return -1;
    }

    LOG(LOG_INFO, "%s():%d - searched for other pwid :  %d !!!!\n", __FUNCTION__, __LINE__, working_pwid);

    if(working_pwid>0){
	LOG(LOG_INFO, "%s():%d - has working pwid, no new pwid are issued (%d)\n" ,__FUNCTION__, __LINE__,working_pwid);
	return len;
    }	

    char other_pwid_str[32];
    if(other_pwid < 0){
	sprintf(other_pwid_str, "NULL");
    } else {
	sprintf(other_pwid_str, "%d", other_pwid);
    }

    result = dbi_conn_queryf(db,  "INSERT INTO  `rembox`.`acc_devices_pwl`"
			     "        (`pwid` ,`atid` ,`id` ,`username` ,`password` ,`enable` ,`exported` ,`used` ,`created` ,`replaced`)"
			     " VALUES (NULL ,  %d,  %d,  '%s', NULL ,  '1',  '0',  '0', CURRENT_TIMESTAMP ,  %s)"
			     , REMACC_TYPE_MAC, boxid, name, other_pwid_str);

    if(result){
	dbi_result_free(result);
    } else {
	const char *err;
        dbi_conn_error(db, &err);
        LOG(LOG_INFO, "%s():%d - dbi error querying database (%s)\n", __FUNCTION__, __LINE__, err);
        return -1;
    } 

    LOG(LOG_INFO, "%s():%d - inserted new pwid :  %d !!!!\n", __FUNCTION__, __LINE__, dbi_conn_sequence_last(db, NULL));


    return len;

}


