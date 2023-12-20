#include <string.h>
#include <stdlib.h>

#include "x_string.h"

static unsigned int
hash32( const char *s, unsigned int l )
{
  unsigned int h = 0;
  for ( unsigned int i = 0; i < l; i++ )
    h = (h << 5) + h + s[ i ];
  if ( h == 0 ) return 1;
  return h;
}

struct Ar_s;
static size_t ar_size, ar_count;
static Ar_s * ar;

struct Ar_s {
  unsigned int len,
               hash;
  char       * str;

  static void resize( void ) {
    size_t osize = ar_size;
    ar_size = ( ar_size == 0 ? 16 : ar_size * 2 );
    ar = (Ar_s *) ::realloc( ar, sizeof( ar[ 0 ] ) * ar_size );
    ::memset( &ar[ osize ], 0, sizeof( ar[ 0 ] ) * ( ar_size - osize ) );

    for ( size_t i = 0; ; i++ ) {
      if ( ar[ i ].str == NULL ) {
        if ( i >= osize )
          break;
      }
      else {
        char       * s = ar[ i ].str;
        unsigned int h = ar[ i ].hash,
                     l = ar[ i ].len;
        size_t       j = h % ar_size;
        if ( j == i )
          continue;
        ar[ i ].str = NULL;
        while ( ar[ j ].str != NULL )
          j = ( j + 1 ) % ar_size;
        ar[ j ].len  = l;
        ar[ j ].hash = h;
        ar[ j ].str  = s;
      }
    }
  }

  static bool locate( unsigned int &h, size_t &i, const char *str, size_t len ){
    h = hash32( str, len );
    i = h % ar_size;
    while ( ar[ i ].str != NULL ) {
      if ( ar[ i ].hash == h && ar[ i ].len == len &&
           ::memcmp( ar[ i ].str, str, len ) == 0 ) {
        return true;
      }
      i = ( i + 1 ) % ar_size;
    }
    return false;
  }
};

unsigned int
X_String::ref( const char *&str,  size_t len )
{
  if ( str == NULL )
    return 0;
  if ( len == 0 ) {
    len = ::strlen( str );
    if ( len == 0 ) {
      str = "";
      return 0;
    }
  }
  if ( ar_count >= ar_size / 2 )
    Ar_s::resize();
  unsigned int h;
  size_t       i;
  if ( Ar_s::locate( h, i, str, len ) ) {
    str = ar[ i ].str;
  }
  else {
    ar[ i ].len  = len;
    ar[ i ].hash = h;
    ar[ i ].str  = (char *) ::malloc( len + 1 );
    ::memcpy( ar[ i ].str, str, len );
    ar[ i ].str[ len ] = '\0';
    str = ar[ i ].str;
    ar_count++;
  }
  return h;
}

unsigned int
X_String::ref2( const char *&str,  size_t &len,
                const char *add,  size_t alen )
{
  size_t sz = len + alen;
  if ( sz == 0 ) {
    str = "";
    len = 0;
    return 0;
  }
  char * s = (char *) ::malloc( sz + 1 );
  ::memcpy( s, str, len );
  ::memcpy( &s[ len ], add, alen );
  s[ len + alen ] = '\0';

  if ( ar_count >= ar_size / 2 )
    Ar_s::resize();

  unsigned int h;
  size_t       i;
  if ( Ar_s::locate( h, i, s, sz ) ) {
    str = ar[ i ].str;
    ::free( s );
  }
  else {
    ar[ i ].len  = len;
    ar[ i ].hash = h;
    ar[ i ].str  = s;
    str = s;
    ar_count++;
  }
  len = sz;
  return h;
}
