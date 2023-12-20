#include <time.h>
#include <sys/time.h>

struct X_PreciseTime {
  static const char *now( void ) {
    static char buf[ 32 ];
    struct timeval tv;
    struct tm tm;
    ::gettimeofday( &tv, NULL );
    ::localtime_r( &tv.tv_sec, &tm );
    ::snprintf( buf, sizeof( buf ),
                "%02u:%02u:%02u.%03u %02u %.3s %04u",
                tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned)( tv.tv_usec / 1000 ),
                tm.tm_mday, "JanFebMarAprMayJunJulAugSepOctNovDec" + tm.tm_mon * 3,
                tm.tm_year + 1900 );
    return buf;
  }
};
