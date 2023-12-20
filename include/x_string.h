
struct X_String {
  const char * str;
  size_t       len;
  unsigned int h;

  X_String( const char *s,  size_t l ) {
    this->h   = X_String::ref( s, l );
    this->str = s;
    this->len = l;
  }
  X_String( const char *s,  size_t l,  const char *a,  size_t al ) {
    this->h = X_String::ref2( s, l, a, al );
    this->str = s;
    this->len = l;
  }
  X_String( size_t = 0 ) : str( 0 ), len( 0 ) {}
  X_String Mid( size_t pos, size_t n = 0 ) {
    X_String s( this->str + pos, n ? n : this->len - pos );
    return s;
  };
  int index( const char *s ) const {
    const char *p = ::strstr( this->str, s );
    return p ? p - this->str : -1;
  }
  int last( char c ) const {
    const char *p = ::strrchr( this->str, c );
    return p ? p - this->str : -1;
  }
  unsigned int hash( void ) const {
    return this->h;
  }
  size_t length( void ) const {
    return this->len;
  }
  bool operator==( size_t zero ) const {
    return this->len == zero && this->str == 0;
  }
  bool operator!=( size_t zero ) const {
    return this->len != zero || this->str != 0;
  }
  bool operator==( const X_String &s ) const {
    return this->len == s.len && ::memcmp( this->str, s.str, this->len ) == 0;
  }
  bool operator!=( const X_String &s ) const {
    return this->len != s.len || ::memcmp( this->str, s.str, this->len ) != 0;
  }
  X_String &operator+=( const X_String &s ) {
    this->h = X_String::ref2( this->str, this->len, s.str, s.len );
    return *this;
  }
  static unsigned int ref( const char *&str,  size_t len );
  static unsigned int ref2( const char *&str,  size_t &len,
                            const char *add,  size_t alen );
};

