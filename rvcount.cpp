//                                                                                                                        
//                                                                                                                        
//                                                  ##                                                                     
//                                                  ##                                                                     
//   ### ###    ##  #####   #####   ##  ##  #####  ####                                                                    
//   #### ##    ## ##   ## ##   ##  ##  ##  ### ##  ##                                                                     
//   ##    ##  ##  ##      ##   ##  ##  ##  ##  ##  ##                                                                     
//   ##    ##  ##  ##      ##   ##  ##  ##  ##  ##  ##                                                                     
//   ##     ####   ##      ##   ##  ##  ##  ##  ##  ##                                                                     
//   ##     ####   ##   ## ##   ##  ## ###  ##  ##  ##                                                                     
//   ##      ##     #####   #####    #####  ##  ##   ##                                                                    
//                                                                                                                        
/*
 * Copyright 1994-1997. ALL RIGHTS RESERVED.
 * TIB/Rendezvous is protected under US Patent No. 5,187,787.
 * For more information, please contact:
 * TIBCO Software, Inc., Palo Alto, California, USA
 *
 * @(#)rvlisten.cpp	1.6
 */

//----------------------------------------------------------------------------------------------------
/* $History: rvcount.cpp $ 
 * 
 * *****************  Version 13  *****************
 * User: Pg50976      Date: 1/10/04    Time: 16:58
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * Added -batchmode argument
 * 
 * *****************  Version 12  *****************
 * User: Pg50976      Date: 1/10/04    Time: 16:41
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * Upgrading project to VC7.1
 * 
 * *****************  Version 11  *****************
 * User: Tr67689      Date: 7/06/04    Time: 9:54
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * Display milliseconds
 * 
 * *****************  Version 10  *****************
 * User: Ts83364      Date: 4 04 03    Time: 9:14
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * synchronise
 * 
 * *****************  Version 9  *****************
 * User: Ts83364      Date: 31 03 03   Time: 18:38
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * fixed
 * 
 * *****************  Version 8  *****************
 * User: Ts83364      Date: 31 03 03   Time: 18:14
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * 
 * *****************  Version 7  *****************
 * User: Ts83364      Date: 31 03 03   Time: 18:09
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * extended subjects
 * 
 * *****************  Version 6  *****************
 * User: Ts83364      Date: 27/06/02   Time: 15:54
 * Updated in $/Mainline/EQLibraries/EQTib/RVCount
 * -lastpart for exchanges
*/
//----------------------------------------------------------------------------------------------------


/*
 * sample rv listening program
 *
 */

#include <stdlib.h>
#include "EQTime.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include <rvcpp.h>
#include <rvevm.h>
#include <iomanip>

#include "windows.h"
#include "EQHash.h"
#include "EQString.h"
#include "EQInteger.h"


//--missing declaration
class EQApplicationFlags
{
private:
	static EQString myAppFlagsRun;
};
EQString EQApplicationFlags::myAppFlagsRun;

class EQLocale
{
public:
	static EQString GetUserShortDateFormat();
};

EQString EQLocale::GetUserShortDateFormat()
{
	TCHAR dateFormat[100] = {0};
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, dateFormat, sizeof(dateFormat) - 1);
	//int retSize = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, dateFormat, sizeof(dateFormat) - 1);
	//if( retSize == 0 )
	//{
	//	DWORD lastErr = GetLastError();
	//	EQString msg = EQString("Failed in GetUserShortDateFormat with error ") + toString( (long)lastErr );
	//	THROWMESSAGE( msg );
	//}
	return dateFormat;
}

//~~missing declaration

class ListenHandler : public RvDataCallback, public RvTimerCallback
{
public:
    ListenHandler( bool bBatchMode ) : RvDataCallback(), myBatchMode( bBatchMode ) { }
    virtual ~ListenHandler() { }
    
    void onData(const char* 	subject,
		RvSender*	replySender,
		const RvDatum&	data,
		RvListener*	invoker);
		
    void onTimeOut(RvListener* invoker);

    virtual void onTimer(RvTimer* invoker);

private:
    bool myBatchMode;

};

class SigHandler : public RvSignalCallback
{
public:
    SigHandler() : RvSignalCallback() { }
    virtual ~SigHandler() { }
    
    void onSignal(RvSignal* invoker);
};

//static
bool ourShowBytes = false;
EQHash< EQString, EQInteger>	ourBytes;
EQHash< EQString, EQInteger>	ourCount;
EQArray< EQString>		ourSubjects;
int parts = 1;
bool lastpart = false;

void
ListenHandler::onData(const char*     csubject,
		      RvSender*       replySender,
		      const RvDatum&  data,
		      RvListener*     /*invoker*/)
{
    //printf("received: subject=%s, reply=%s, message=",
    //    subject, replySender!=NULL ? replySender->subject() : "<none>");
    //data.print();
    //printf("\n");
    //fflush(stdout);
    EQString subject(csubject);
    int dot = 0;
    for(int i=0; i<parts; i++)
	dot += 1+subject.Mid(dot+1).index(".");
    EQString sub = subject(0, dot);

    //now get lastpart
    if(lastpart)
	sub += subject.Mid(subject.last('.'));

    int len  = (int) data.size();	// get # of bytes
    if(data.type()==RVMSG_RVMSG)
    {
	// special handling for msg type to get correct # of bytes
	RvMsg msg((rvmsg_Msg *) data.data());	
	len = msg.length();
    }
    ourBytes[sub] += len;
    ourCount[sub] += 1;

    if(len > 1000)
	;//cout << "Huge packet: " << len << "b \t" << subject << endl;

    static const EQString listenmsg("_RV.INFO.SYSTEM.LISTEN");
    static const EQString rvstr("_RV");
    if( EQString(subject)(0,3)==rvstr && EQString(subject)(0, listenmsg.length()) != listenmsg ) {
	;//cout << subject << endl;
	//Sleep(500);
    }
}

void
ListenHandler::onTimeOut(RvListener* invoker)
{
    printf("listen timed out on %s\n", invoker->subject());
    delete invoker->session();
}

void
SigHandler::onSignal(RvSignal* invoker)
{
    const int ms=10000;
    printf("Sleeping for %d sec\n", ms/1000);
    Sleep(ms);
}

void ListenHandler::onTimer(RvTimer* invoker)
{
    cout << EQPreciseTime::now() << "\t"; //.Format("%X\t");
    EQHashIterator< EQString, EQInteger> iter(ourBytes);
    while(++iter)
	ourSubjects.addUnique(iter.key());    //keeps displayed data in same order that it arrives
    for(int i=0; i<ourSubjects.count(); i++)
    {
	const EQString& subject = ourSubjects[i];
	int b = ourBytes[subject], c=ourCount(subject);
	cout << subject << ":";
	if(ourShowBytes)
	    cout << setw(5) << b << "b";
	cout << setw(4) << c ;
	if(ourShowBytes)
	    cout << "p" << setw(4) << b/(c?c:1) << "a";
	cout << " ";
	ourBytes[subject]=0;
	ourCount[subject]=0;
    }
    cout << endl;

    if( myBatchMode )
    {
	// Time to quit
	rv_Term( invoker->session()->rv_session() );
    }
}

//======================================================================================== 

void usage()
{
    fprintf(stderr,"rvlisten [-parts n /*check first n parts of the subject*/]");
    fprintf(stderr,"	[-lastpart /*include part of subject, e.g. exchange] [-bytes] ");
    fprintf(stderr,"    [-service service] [-network network] \n");
    fprintf(stderr,"	[-daemon daemon] [-timeout milliseconds]\n");
    fprintf(stderr,"	[-batchmode]\n");
    fprintf(stderr,"	[-subjects subjectfile]\n");
    fprintf(stderr,"	<subject list>\n");
    exit(-1);
}

void main(int argc, char *argv[])
{
    rv_Error            err;
    int                 i;
    rv_Session          rvsess;

    char                *service=NULL;
    char                *network=NULL;
    char                *daemon=NULL;  

    unsigned long       timeOut = 1000;
    char                *subjectfile=NULL;  

    bool		isBatchMode=false;
    
    i = 1;
    if( argc > 1 )
    {
	while ( i < argc && *argv[i] == '-' )
	{
	    if(strcmp(argv[i], "-service") == 0)
	    {
		service = argv[i+1];
		i+=2;
	    }
	    else if(strcmp(argv[i], "-network") == 0)
	    {
		network = argv[i+1];
		i+=2;
	    }
	    else if(strcmp(argv[i], "-daemon") == 0)
	    {
		daemon = argv[i+1];
		i+=2;
	    }
	    else if(strcmp(argv[i], "-timeout") == 0)
	    {
		timeOut = atol(argv[i+1]);
		i+=2;
	    }
	    else if(strcmp(argv[i], "-parts") == 0)
	    {
		parts = atol(argv[i+1]);
		i+=2;
	    }
	    else if(strcmp(argv[i], "-lastpart") == 0)
	    {
		lastpart = true;
		i+=1;
	    }
	    else if(strcmp(argv[i], "-bytes") == 0)
	    {
		ourShowBytes = true;
		i++;
	    }
	    else if(strcmp(argv[i], "-subjects") == 0)
	    {
		subjectfile = argv[i+1];
		i+=2;
	    }
	    else if(strcmp(argv[i], "-batchmode") == 0)
	    {
		isBatchMode = true;
		i++;
	    }
	    else
	    {
		usage();
	    }
	}
    }
    
    err = rv_Init(&rvsess, service, network, daemon);
    if(err != RV_OK)
    {
	fprintf(stderr, "%s: Failed to initialize session--%s\n",
		argv[0], rv_ErrorText(NULL, err));
	exit(1);
    }

    RvSession*     sess = new RvSession(rvsess);
    ListenHandler* lhandler;
    SigHandler*    shandler;
    RvListener*    l;
    
    if (sess->status() != RV_OK)
    {
	fprintf(stderr, "%s: Failed to initialize RvSession--%s\n",
		argv[0], sess->status().description());
	exit(1);
    }
    
    if (((lhandler = new ListenHandler( isBatchMode )) == NULL) ||
        ((shandler = new SigHandler()) == NULL))
    {
	fprintf(stderr, "%s: Out of memory\n", argv[0]);
	exit(1);
    }

    if (sess->newSignal(SIGINT, shandler) == NULL)
    {
	fprintf(stderr, "%s: Failed to initialize RvSignal--%s\n",
		argv[0], sess->status().description());
	exit(1);
    }
    
    if(subjectfile!=NULL)
    {
        ifstream subjects(subjectfile);
	char oneSubj[1000];
	while(!subjects.eof())
	{
	    subjects.getline(oneSubj, 1000);
	    if ((l =  sess->newListener(oneSubj, 0, lhandler)) == NULL)
                fprintf(stderr, "%s: error %s listening to \"%s\"\n",
                    argv[0], sess->status().description(), oneSubj);
	    else
		fprintf(stderr, "%s: listening to \"%s\"\n",
		    argv[0], l->subject());
	}
    }
    else if (argc == i)
    {
    	if ((l =  sess->newListener(">",1000, lhandler)) == NULL)
             fprintf(stderr, "%s: error %s listening to INBOX\n",
                argv[0], sess->status().description());
        else
            fprintf(stderr, "%s: listening to \"%s\"\n",
                argv[0], l->subject());
    }
    else 
    {
        for (; i<argc; i++) 
	{
	    if ((l =  sess->newListener(argv[i], 0, lhandler)) == NULL)
                fprintf(stderr, "%s: error %s listening to \"%s\"\n",
                    argv[0], sess->status().description(), argv[i]);
	    else
		fprintf(stderr, "%s: listening to \"%s\"\n",
		    argv[0], l->subject());
        }
    }
    SYSTEMTIME time;
    GetSystemTime(&time);
    Sleep(1000-time.wMilliseconds);
    if (sess->newTimer(timeOut, /*repeat*/ RV_TRUE, lhandler) == NULL)
    {
	fprintf(stderr, "%s: Failed to initialize Timer--%s\n",
		argv[0], sess->status().description());
	exit(1);
    }
    if(ourShowBytes)
	cout << "b(ytes/sec), p(ackets/sec), a(verage)" << endl;
    else
	cout << "packets/sec" << endl;

    rv_MainLoop(rvsess);
    
} // main


