/*
 * Copyright (c) 2004, Apple Computer, Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libbsm.h>

/* 
 * Parse the contents of the audit_control file to return 
 * the audit control parameters
 */  
static FILE *fp = NULL;
static char linestr[AU_LINE_MAX];
static char *delim = ":";

static char inacdir = 0;
static char ptrmoved = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* 
 * Returns the string value corresponding to the given label  
 * from the configuration file
 */  
static int getstrfromtype(char *name, char **str)
{
	char *type, *nl;
	char *tokptr;
	char *last;
	
	*str = NULL;

	pthread_mutex_lock(&mutex);

	if((fp == NULL) 
		&& ((fp = fopen(AUDIT_CONTROL_FILE, "r")) == NULL)) {

		pthread_mutex_unlock(&mutex);
		return 0; /* Error */
	}

	/* Search for the line beginning with the given name */
	while(fgets(linestr, AU_LINE_MAX, fp) != NULL) {

		/* Remove trailing new line character */
		if((nl = strrchr(linestr, '\n')) != NULL) {
			*nl = '\0';
		}

		tokptr = linestr;
		if((type = strtok_r(tokptr, delim, &last)) != NULL) {
			if(!strcmp(name, type)) {

				/* Found matching name */
				*str = strtok_r(NULL, delim, &last);

				pthread_mutex_unlock(&mutex);

				if(*str == NULL) {
					return 1; /* Parse error in file */
				}
				return 0; /* Success */
			}
		}	
	}

	pthread_mutex_unlock(&mutex);
	return 0; /* EOF */
}

/* 
 * Rewind the file pointer to beginning 
 */  
void setac()
{
	pthread_mutex_lock(&mutex);

	ptrmoved = 1;
	if(fp != NULL) {
		fseek(fp, 0, SEEK_SET);
	}

	pthread_mutex_unlock(&mutex);
}

/*
 * Close the audit_control file
 */  
void endac()
{
	pthread_mutex_lock(&mutex);
	
	ptrmoved = 1;
	if(fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	pthread_mutex_unlock(&mutex);
}

/*
 * Return audit directory information from the audit control file
 */  
int getacdir(char *name, int len)
{
	char *dir;
	int ret = 0;
	
	if(name == NULL) {
		errno = EINVAL;
		return -2;
	}

	pthread_mutex_lock(&mutex);

	/* 
	 * Check if another function was called between 
	 * successive calls to getacdir 
	 */
	if(inacdir && ptrmoved) {
		ptrmoved = 0;
		if(fp != NULL) {
			fseek(fp, 0, SEEK_SET);	
		}
		
		ret = 2; 
	}
					
	pthread_mutex_unlock(&mutex);
	
	if(getstrfromtype(DIR_CONTROL_ENTRY, &dir) == 1) {
		return -3;
	}
			
	if(dir == NULL){

		return -1;
	}

	if(strlen(dir) >= len) {
		return -3;
	}

	strcpy(name, dir);	

	return ret;
}

/*
 * Return the minimum free diskspace value from the audit control file
 */  
int getacmin(int *min_val)
{
	char *min;

	setac();
	
	if(min_val == NULL) {
		errno = EINVAL;
		return -2;
	}

	if(getstrfromtype(MINFREE_CONTROL_ENTRY, &min) == 1) {
		return -3;
	}
	
	if(min == NULL) {
		return 1;
	}

	*min_val = atoi(min);

	return 0;
}

/*
 * Return the system audit value from the audit contol file
 */  
int getacflg(char *auditstr, int len)
{
	char *str;

	setac();
		
	if(auditstr == NULL) {
		errno = EINVAL;
		return -2;
	}

	if(getstrfromtype(FLAGS_CONTROL_ENTRY, &str) == 1) {
		return -3;
	}
	
	if(str == NULL) {
		return 1;
	}

	if(strlen(str) >= len) {
		return -3;
	}

	strcpy(auditstr, str);	

	return 0;
}


/*
 * Return the non attributable flags from the audit contol file
 */  
int getacna(char *auditstr, int len)
{
	char *str;

	setac();
		
	if(auditstr == NULL) {
		errno = EINVAL;
		return -2;
	}

	if(getstrfromtype(NA_CONTROL_ENTRY, &str) == 1) {
		return -3;
	}

	if(str == NULL) {
		return 1;
	}

	if(strlen(str) >= len) {
		return -3;
	}

	strcpy(auditstr, str);	

	return 0;
}

