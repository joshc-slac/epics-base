/*	$Id$
 *	Author:	Roger A. Cole
 *	Date:	07-20-91
 *
 *	Experimental Physics and Industrial Control System (EPICS)
 *
 *	Copyright 1991, the Regents of the University of California,
 *	and the University of Chicago Board of Governors.
 *
 *	This software was produced under  U.S. Government contracts:
 *	(W-7405-ENG-36) at the Los Alamos National Laboratory,
 *	and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *	Initial development by:
 *		The Controls and Automation Group (AT-8)
 *		Ground Test Accelerator
 *		Accelerator Technology Division
 *		Los Alamos National Laboratory
 *
 *	Co-developed with
 *		The Controls and Computing Group
 *		Accelerator Systems Division
 *		Advanced Photon Source
 *		Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01	07-20-91	rac	initial version
 * .02	08-07-91	joh	added config get for long and double C types
 * .03	08-07-91	joh	added config get for struct in_addr type
 * .04	01-11-95	joh	use getenv()/putenv() to fetch/write env 
 *				vars under vxWorks	
 * .05  04-20-95	anj	changes to use CONFIG_ENV
 * .06  05-24-95	joh	added return stmnt to epicsPrtEnvParams()	
 * .07  11-03-96	joh	fixed bug occuring when diagnostic is
 *				printed and the env var cant be found
 * .08  09-11-96        joh     ANSI prototypes
 * .09  10-18-96        joh    	added envParamIsEmpty() 
 *
 * make options
 *	-DvxWorks	makes a version for VxWorks
 *	-DNDEBUG	don't compile assert() checking
 *      -DDEBUG         compile various debug code, including checks on
 *                      malloc'd memory
 */
/*+/mod***********************************************************************
* TITLE	envSubr.c - routines to get and set EPICS environment parameters
*
* DESCRIPTION
*	These routines are oriented for use with EPICS environment
*	parameters under UNIX and VxWorks.  They may be used for other
*	purposes, as well.
*
*	Many EPICS environment parameters are predefined in envDefs.h.
*
* QUICK REFERENCE
* #include <envDefs.h>
* ENV_PARAM	param;
*  char *envGetConfigParam(          pParam,    bufDim,    pBuf       )
*  long  envGetLongConfigParam(      pParam,    pLong                 )
*  long  envGetDoubleConfigParam(    pParam,    pDouble               )
*  long  envGetInetAddrConfigParam(  pParam,    pAddr                 )
*  long  envPrtConfigParam(          pParam                           )
*  long  envSetConfigParam(          pParam,    valueString           )
*  int envParamIsEmpty(		     pParam			      )
)
*
* SEE ALSO
*	$epics/share/bin/envSetupParams, envDefs.h
*
*-***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef vxWorks
#include <inetLib.h>
#include <envLib.h>
#include <errnoLib.h>
#endif

#include "errMdef.h"
#define epicsExportSharedSymbols
#include "envDefs.h"



/*+/subr**********************************************************************
* NAME	envGetConfigParam - get value of a configuration parameter
*
* DESCRIPTION
*	Gets the value of a configuration parameter and copies it
*	into the caller's buffer.  If the configuration parameter
*	isn't found in the environment, then the default value for
*	the parameter is copied.  If no parameter is found and there
*	is no default, then '\0' is copied and NULL is returned.
*
* RETURNS
*	pointer to callers buffer, or
*	NULL if no parameter value or default value was found
*
* EXAMPLES
* 1.	Get the value for the EPICS-defined environment parameter
*	EPICS_TS_MIN_WEST.
*
*	#include <envDefs.h>
*	char       temp[80];
*
*	printf("minutes west of UTC is: %s\n",
*			envGetConfigParam(&EPICS_TS_MIN_WEST, 80, temp));
*
* 2.	Get the value for the DISPLAY environment parameter under UNIX.
*
*	#include <envDefs.h>
*	char       temp[80];
*	ENV_PARAM  display={"DISPLAY",""}
*
*	if (envGetConfigParam(&display, 80, temp) == NULL)
*	    printf("DISPLAY isn't defined\n");
*	else
*	    printf("DISPLAY is %s\n", temp);
*
*-*/
char * epicsShareAPI envGetConfigParam(
ENV_PARAM *pParam,	/* I pointer to config param structure */
int	bufDim,		/* I dimension of parameter buffer */
char	*pBuf		/* I pointer to parameter buffer  */
)
{
    char	*pEnv;		/* pointer to environment string */

    pEnv = getenv(pParam->name);

    if (pEnv == NULL) {
	pEnv = pParam->dflt;
    }

    if (pEnv[0u] == '\0') {
	return NULL;
    }

    strncpy(pBuf, pEnv, bufDim-1);
    pBuf[bufDim-1] = '\0';

    return pBuf;
}

/*+/subr**********************************************************************
* NAME	envGetDoubleConfigParam - get value of a double configuration parameter
*
* DESCRIPTION
*	Gets the value of a configuration parameter and copies it into the
*	caller's real (double) buffer.  If the configuration parameter isn't
*	found in the environment, then the default value for the parameter
*	is copied.  
*
*	If no parameter is found and there is no default, then -1 is 
*	returned and the caller's buffer is unmodified.
*
* RETURNS
*	0, or
*	-1 if an error is encountered
*
* EXAMPLE
* 1.	Get the value for the real environment parameter EPICS_THRESHOLD.
*
*	#include <envDefs.h>
*	double	threshold;
*	long	status;
*
*	status = envGetDoubleConfigParam(&EPICS_THRESHOLD, &threshold);
*	if (status == 0) {
*	    printf("the threshold is: %lf\n", threshold);
*	}
*	else {
*	    printf("%s could not be found or was not a real number\n",
*			EPICS_THRESHOLD.name);
*	}
*
*-*/
long epicsShareAPI envGetDoubleConfigParam(
ENV_PARAM *pParam,	/* I pointer to config param structure */
double	*pDouble	/* O pointer to place to store value */
)
{
    char	text[128];
    char	*ptext;
    int		count;

    ptext = envGetConfigParam(pParam, sizeof text, text);
    if (ptext != NULL) {
	count = sscanf(text, "%lf", pDouble);
	if (count == 1) {
	    return 0;
	}
        (void)printf("Unable to find a real number in %s=%s\n", 
		pParam->name, text);
    }

    return -1;
}

/*+/subr**********************************************************************
* NAME	envGetInetAddrConfigParam - get value of an inet addr config parameter
*
* DESCRIPTION
*	Gets the value of a configuration parameter and copies it into
*	the caller's (struct in_addr) buffer.  If the configuration parameter
*	isn't found in the environment, then the default value for
*	the parameter is copied.  
*
*	If no parameter is found and there is no default, then -1 is 
*	returned and the callers buffer is unmodified.
*
* RETURNS
*	0, or
*	-1 if an error is encountered
*
* EXAMPLE
* 1.	Get the value for the inet address environment parameter EPICS_INET.
*
*	#include <envDefs.h>
*	struct	in_addr	addr;
*	long	status;
*
*	status = envGetInetAddrConfigParam(&EPICS_INET, &addr);
*	if (status == 0) {
*	    printf("the s_addr is: %x\n", addr.s_addr);
*	}
*	else {
*	    printf("%s could not be found or was not an inet address\n",
*			EPICS_INET.name);
*	}
*
*-*/
long epicsShareAPI envGetInetAddrConfigParam(
ENV_PARAM *pParam,	/* I pointer to config param structure */
struct in_addr *pAddr	/* O pointer to struct to receive inet addr */
)
{
    char	text[128];
    char	*ptext;
    long	status;

    ptext = envGetConfigParam(pParam, sizeof text, text);
    if (ptext) {
	status = inet_addr (text);
	if (status != -1) {
	    pAddr->s_addr = status;
	    return 0;
	}
        (void)printf("Unable to find an IP address in %s=%s\n", 
		pParam->name, text);
    }
    return -1;
}

/*+/subr**********************************************************************
* NAME	envGetLongConfigParam - get value of an integer config parameter
*
* DESCRIPTION
*	Gets the value of a configuration parameter and copies it
*	into the caller's integer (long) buffer.  If the configuration 
*	parameter isn't found in the environment, then the default value for
*	the parameter is copied.  
*
*	If no parameter is found and there is no default, then -1 is 
*	returned and the callers buffer is unmodified.
*
* RETURNS
*	0, or
*	-1 if an error is encountered
*
* EXAMPLE
* 1.	Get the value as a long for the integer environment parameter
*	EPICS_NUMBER_OF_ITEMS.
*
*	#include <envDefs.h>
*	long	count;
*	long	status;
*
*	status = envGetLongConfigParam(&EPICS_NUMBER_OF_ITEMS, &count);
*	if (status == 0) {
*	    printf("and the count is: %d\n", count);
*	}
*	else {
*	    printf("%s could not be found or was not an integer\n",
*			EPICS_NUMBER_OF_ITEMS.name);
*	}
*
*-*/
long epicsShareAPI envGetLongConfigParam(
ENV_PARAM *pParam,	/* I pointer to config param structure */
long	*pLong		/* O pointer to place to store value */
)
{
    char	text[128];
    char	*ptext;
    int		count;

    ptext = envGetConfigParam(pParam, sizeof text, text);
    if (ptext) {
	count = sscanf(text, "%ld", pLong);
	if (count == 1)
		return 0;
        (void)printf("Unable to find an integer in %s=%s\n", 
		pParam->name, text);
    }
    return -1;
}

/*+/subr**********************************************************************
* NAME	envPrtConfigParam - print value of a configuration parameter
*
* DESCRIPTION
*	Prints the value of a configuration parameter.
*
* RETURNS
*	0
*
* EXAMPLE
* 1.	Print the value for the EPICS-defined environment parameter
*	EPICS_TS_MIN_WEST.
*
*	#include <envDefs.h>
*
*	envPrtConfigParam(&EPICS_TS_MIN_WEST);
*
*-*/
long epicsShareAPI envPrtConfigParam(
ENV_PARAM *pParam)	/* pointer to config param structure */
{
    char	text[80];
    if (envGetConfigParam(pParam, 80, text) == NULL)
	printf("%s is undefined\n", pParam->name);
    else
	printf("%s: %s\n", pParam->name, text);
    return 0;
}

/*+/subr**********************************************************************
* NAME	envSetConfigParam - set value of a configuration parameter
*
* DESCRIPTION
*	Sets the value of a configuration parameter.
*
* RETURNS
*	0
*
* NOTES
* 1.	Performs a useful function only under VxWorks.
*
* EXAMPLE
* 1.	Set the value for the EPICS-defined environment parameter
*	EPICS_TS_MIN_WEST to 360, for USA central time zone.
*
*	Under UNIX:
*
*		% setenv EPICS_TS_MIN_WEST 360
*
*	In a program running under VxWorks:
*
*		#include <envDefs.h>
*
*		envSetConfigParam(&EPICS_TS_MIN_WEST, "360");
*
*	Under the VxWorks command shell:
*
*		envSetConfigParam &EPICS_TS_MIN_WEST,"360"
*
*-*/
long epicsShareAPI envSetConfigParam (
ENV_PARAM 	*pParam,	/* I pointer to config param structure */
char		*value		/* I pointer to value string */
)
{
#ifndef vxWorks
    	printf("envSetConfigParam can only be used under vxWorks\n");
	return -1L;
#else
	long	retCode = 0;
	int	status;
	char	*pEnv;

	/*
	 * space for two strings, an '=' character,
	 * and a null termination
	 */
	pEnv = malloc (strlen (pParam->name) + strlen (value) + 2);
	if (!pEnv) {
		errPrintf(
			-1L,
			__FILE__,
			__LINE__,
"Failed to set environment parameter \"%s\" to \"%s\" because \"%s\"\n",
			pParam->name,
			value,
			strerror (errnoGet()));
		return -1L;
	}

	strcpy (pEnv, pParam->name);
	strcat (pEnv, "=");
	strcat (pEnv, value);
	status = putenv (pEnv);
	if (status<0) {
		errPrintf(
			-1L,
			__FILE__,
			__LINE__,
"Failed to set environment parameter \"%s\" to \"%s\" because \"%s\"\n",
			pParam->name,
			value,
			strerror (errnoGet()));
		retCode = -1L;
	}
	/*
 	 * vxWorks copies into a private buffer
	 * (this does not match UNIX behavior)
	 */
	free (pEnv);
	
	return retCode;
#endif
}

/*+/subr**********************************************************************
* NAME  epicsPrtEnvParams - print value of all configuration parameters
*
* DESCRIPTION
*       Prints all configuration parameters and their current value.
*
* RETURNS
*       0
*
* EXAMPLE
* 1.    Print the value for all EPICS-defined environment parameters.
*
*       #include <envDefs.h>
*
*       epicsPrtEnvParams();
*
*-*/
long
epicsPrtEnvParams()
{
    ENV_PARAM **ppParam = env_param_list;
     
    while (*ppParam != NULL)
	envPrtConfigParam(*(ppParam++));

    return 0;
}

/*+/subr**********************************************************************
* NAME  envParamIsEmpty - test for empty environment parameter 
*
* DESCRIPTION
*	Return TRUE if the environment parameter is empty - else false
*
* AUTHOR
*	Jeff Hill
* RETURNS
*	0 or 1
*
* EXAMPLE
*       #include <envDefs.h>
*
*       if (envParamIsEmpty(&EPICS_CAS_SERVER_PORT)) {
*		 port = caFetchPortConfig(&EPICS_CA_SERVER_PORT, CA_SERVER_PORT);
*	}
*
*-*/
int envParamIsEmpty(
ENV_PARAM *pParam	/* I pointer to config param structure */
)
{
    char	*pEnv;		/* pointer to environment string */

    pEnv = getenv(pParam->name);

    if (pEnv == NULL && pParam->dflt[0u] == '\0') {
	return 1;
    }
    else {
	return 0;
    }
}

