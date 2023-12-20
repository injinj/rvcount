#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fstream>
#include <sassrv/rvcpp.h>

#include "x_time.h"
#include "x_hash.h"
#include "x_string.h"
#include "x_integer.h"

using namespace std;

ostream & operator << ( ostream &os, const X_String &s ) {
  return os << s.str;
}

class ListenHandler : public RvDataCallback, public RvTimerCallback
{
public:
    ListenHandler( bool bBatchMode )
      : RvDataCallback(), myBatchMode( bBatchMode ) {}
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

bool verbose = false, ourShowBytes = false;
X_Hash< X_String, X_Integer>	ourBytes;
X_Hash< X_String, X_Integer>	ourCount;
X_Array< X_String>		ourSubjects;
int parts = 1;
bool lastpart = false;

void
ListenHandler::onData(const char*     csubject,
		      RvSender*       replySender,
		      const RvDatum&  data,
		      RvListener*     /*invoker*/)
{
  if ( verbose ) {
    printf("received: subject=%s, reply=%s, message=",
        csubject, replySender!=NULL ? replySender->subject() : "<none>");
    data.print();
    printf("\n");
    fflush(stdout);
  }
  X_String subject(csubject, ::strlen(csubject));
  int dot = 0;
  for(int i=0; i<parts; i++)
      dot += 1+subject.Mid(dot+1).index(".");
  X_String sub = subject.Mid(0, dot);
  //now get lastpart
  if(lastpart)
      sub += subject.Mid(subject.last('.'));
  int len  = (int) data.size();	// get # of bytes
#if 0
    if(data.type()==RVMSG_RVMSG)
    {
	// special handling for msg type to get correct # of bytes
	RvMsg msg((rvmsg_Msg *) data.data());	
	len = msg.length();
    }
#endif
  ourBytes[sub] += len;
  ourCount[sub] += 1;
#if 0
    if(len > 1000)
	;//cout << "Huge packet: " << len << "b \t" << subject << endl;

    static const X_String listenmsg("_RV.INFO.SYSTEM.LISTEN");
    static const X_String rvstr("_RV");
    if( X_String(subject)(0,3)==rvstr && X_String(subject)(0, listenmsg.length()) != listenmsg ) {
	;//cout << subject << endl;
	//Sleep(500);
    }
#endif
}

void
ListenHandler::onTimeOut(RvListener* invoker)
{
    printf("listen timed out on %s\n", invoker->subject());
    /*delete invoker->session();*/
}

void
SigHandler::onSignal(RvSignal* invoker)
{
#if 0
    const int ms=10000;
    printf("Sleeping for %d sec\n", ms/1000);
    Sleep(ms);
#endif
    rv_Term( invoker->session()->rv_session() );
}

static char *ip_str( rv_ipaddr_t ip, char *buf, size_t buflen ) {
  snprintf( buf, buflen, "%d.%d.%d.%d",
           ( ip >> 24 ) & 0xff, ( ip >> 16 ) & 0xff,
           ( ip >> 8 ) & 0xff, ip & 0xff );
  return buf;
}

void ListenHandler::onTimer(RvTimer* invoker)
{
    printf( "%s\t", X_PreciseTime::now() );
    X_HashIterator< X_String, X_Integer> iter(ourBytes);
    while(++iter)
	ourSubjects.addUnique(iter.key());    //keeps displayed data in same order that it arrives
    for(size_t i=0; i<ourSubjects.count(); i++)
    {
	const X_String& subject = ourSubjects[i];
	long b = ourBytes[subject], c=ourCount[subject];
        printf( "%s%s: %6ldb %4ldp %4lda ", i == 0 ? "" : "; ", subject.str, b, c, b/(c?c:1) );
	ourBytes[subject]=0;
	ourCount[subject]=0;
    }
    struct rv_Stats stats;
    rv_GetStats( invoker->session()->rv_session(), &stats );
    if ( stats.bytes_recv != 0 ) {
      long b = stats.bytes_recv,
           c = stats.msgs_recv;
      printf( "%s: %6ldb %4ldp %4lda ", "; RV", b, c, b/(c?c:1) );
      char buf[ 32 ];
      if ( stats.lost_seqno != 0 ) {
        printf( "%4ldl ", (long) stats.lost_seqno );
        for ( size_t j = 0; j < stats.lost_src_count; j++ ) {
          printf( "%s ", ip_str( stats.lost_src[ j ], buf, sizeof( buf ) ) );
        }
      }
      if ( stats.repeat_seqno != 0 ) {
        printf( "%4ldr ", (long) stats.repeat_seqno );
        for ( size_t j = 0; j < stats.repeat_src_count; j++ ) {
          printf( "%s ", ip_str( stats.repeat_src[ j ], buf, sizeof( buf ) ) );
        }
      }
      if ( stats.nak_count != 0 ) {
        printf( "%4ldn ", (long) stats.nak_count );
        for ( size_t j = 0; j < stats.nak_src_count; j++ ) {
          printf( "%s ", ip_str( stats.nak_src[ j ], buf, sizeof( buf ) ) );
        }
      }
      if ( stats.reorder_seqno != 0 ) {
        printf( "%4ldo ", (long) stats.reorder_seqno );
        for ( size_t j = 0; j < stats.reorder_src_count; j++ ) {
          printf( "%s ", ip_str( stats.reorder_src[ j ], buf, sizeof( buf ) ) );
        }
      }
    }
    printf( "\n" );

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

int main(int argc, char *argv[])
{
    int                 err;
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
	    else if(strcmp(argv[i], "-verbose") == 0)
	    {
		verbose = true;
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
#if 0
    SYSTEMTIME time;
    GetSystemTime(&time);
    Sleep(1000-time.wMilliseconds);
#endif
    if (sess->newTimer(timeOut, /*repeat*/ RV_TRUE, lhandler) == NULL)
    {
	fprintf(stderr, "%s: Failed to initialize Timer--%s\n",
		argv[0], sess->status().description());
	exit(1);
    }
    if(ourShowBytes)
	printf( "b(ytes/sec), p(ackets/sec), a(verage)\n" );
    else
	printf( "packets/sec\n" );

    rv_MainLoop(rvsess);
    return 0;
} // main


