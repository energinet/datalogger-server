//   ------------------------------------------------------------------------------------- 
/**             Functions for file handeling
 * 
 *   \remarks   Copyright: (c) 2009 LIAB Aps
 *   \remarks   Compiler: arm-softfloat-linux-gnu (Build by LIAB)
 *   \file      rpserver_db.h
 *   \author    CRA
 *///------------------------------------------------------------------------------------- 



#ifndef RPFILE_UTIL_H_
#define RPFILE_UTIL_H_
#include <sys/types.h>
#include <sys/stat.h>


int rpfile_md5(unsigned char * testBuffer, size_t testLen, char* obuf);

int rpfile_md5chk(unsigned char * testBuffer, size_t testLen, char* md5sum);


int rpfile_read(const char *path, char **buffer, size_t *size, mode_t *mode);

int rpfile_write(const char *path, char *buffer, size_t size, mode_t mode);


#endif /* RPFILE_UTIL_H_ */
