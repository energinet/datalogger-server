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



#ifndef RPSERVER_DB_H_
#define RPSERVER_DB_H_
#include <dbi/dbi.h>

#include <remboxdb.h>

#define CMDUSER_BOX -100

/* typedef struct { */
/*     dbi_conn db; */
/*     int drivercount; */
/*     int iid; */
/* } sDSWebservice; */

int rpserver_db_connect(sDSWebservice *ds);


void rpserver_db_disconnect(sDSWebservice *ds);

int rpserver_db_authenticate(dbi_conn db, const char *user, const char *password);

/**
 * Prepare db time format from time_t
 */
int rpserver_db_prep_time(char *strtime, int len,  time_t time);

/**
 * Prepare db time format from struct timeval
 */
int rpserver_db_prep_time_tv(char *strtime, int len, struct timeval *time);

/**
 * Prepare db time format from string 
 */
int rpserver_db_prep_time_str(char *strtime, int len, const char *ptime);



int rpclient_db_eid_get(dbi_conn *db, int iid, int device_id, const char *ename, int *evt_id);

int rpclient_db_iid_get(dbi_conn *db, const char *name);

int rpclient_db_devid_get(dbi_conn *db, const char *name);


int rpserver_db_etype_update(dbi_conn *db, int iid, int device_id, int eid, const char *ename, const char *hname,
			     const char *unit, const char *type);



int rpserver_db_log_add(dbi_conn *db, int iid, int eid, int evt_id, char* ptime , char *value);

/**
 * Update idata
 * idata is: contact   : contact time
 *           localip   : local ip on box
 *           externalip: external ip (received by server)
 *           uptime    : box uptime
 */
//int rpserver_db_idate_update(dbi_conn *db, int iid, struct boxInfo *boxinfo, const char* externalip );
//int rpserver_db_idata_update(dbi_conn *db, int iid, const char *localip, int uptime, const char* externalip );
//int rpserver_db_idata_update(dbi_conn *db, int iid, const char *localip, int uptime, const char* externalip , const char *rpversion , const char *localid);

int rpserver_db_sink_get(dbi_conn *db, const char *address);

int rpserver_db_idata_update(dbi_conn *db, int iid, const char *name,  const char *localip, int uptime, const char* externalip , const char *rpversion , const char *localid, int rpsid);

/**
 * Update update time
 */
//int rpserver_db_updtme_update(dbi_conn *db, int iid, int eid, const char *ename, const char *ptime);
int rpserver_db_updtme_update(dbi_conn *db, int eid, const char *ptime);


//int rpserver_db_cmd_cid_next(dbi_conn *db, int iid);
int rpserver_db_cmd_cid_next(dbi_conn *db);

/**
 * Add command/value to list
 * @param name is command name
 * @parma param is the parameters
 * @param ttime is trigger time (if NULL then current time)
 * @param pseq is 0 if no prevous command or the sequence number of the previous
 * @return sequence number if sucess of <=0 if error;
 */
int rpserver_db_cmd_add(dbi_conn *db, int iid, const char* name, const char* param, 
			struct timeval *ttime, int pseq, int user);



/**
 * Get command status 
 * @param db is dbi database 
 * @param iid is the installaton id
 * @param cid is the command id
 * @return command status
 */
//int rpserver_db_cmd_status(dbi_conn *db, int iid, int cid);
int rpserver_db_cmd_status(dbi_conn *db, int cid);
//int rpserver_db_cmd_clean(dbi_conn *db, int iid, const char* name);


int rpserver_db_conf_get(dbi_conn *db, int iid, const char *confname, char **buffer, size_t *size);
//int rpserver_db_conf_get(dbi_conn *db, int iid, const char *confname, char *buf, int maxlen);
//int rpserver_db_conf_contdaem(dbi_conn *db, int iid, char* buf , int maxlen);

//int rpserver_db_conf_licon(dbi_conn *db, int iid, char *buf, int maxlen);

/*--- PAIR FUNCTIONS ---*/

int rpserver_db_pair_lookup(dbi_conn *db, const char *localid, char *buffer, size_t size);

int rpserver_db_pair_exec(dbi_conn *db, const char *localid, const char *name, char *buffer, size_t size);

int rpserver_db_pair_idcheck(char *buffer);

#endif /* RPSERVER_DB_H_ */
