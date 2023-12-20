
template <class T, class V> struct X_Hash {
  size_t used,
         capacity;
  T    * keys;
  V    * values;
  X_Hash() : used( 0 ), capacity( 0 ), keys( 0 ), values( 0 ) {}
  ~X_Hash() {
    if ( this->capacity > 0 ) {
      delete [] this->keys;
      delete [] this->values;
    }
  }
  bool find( const T &key, size_t &pos ) const {
    if ( this->capacity == 0 ) {
      pos = 0;
      return false;
    }
    size_t i = key.hash() % this->capacity;
    while ( this->keys[ i ] != key ) {
      if ( this->keys[ i ] == 0 ) {
        pos = i;
        return false;
      }
      i = ( i + 1 ) % this->capacity;
    }
    pos = i;
    return true;
  }
  V & operator[]( const T &key ) {
    size_t pos;
    for (;;) {
      if ( this->find( key, pos ) )
        return this->values[ pos ];
      if ( this->used < this->capacity / 2 ) {
        this->keys[ pos ] = key;
        this->used++;
        return this->values[ pos ];
      }
      this->grow();
    }
  }
  void grow( void ) {
    size_t old_capacity = this->capacity;
    this->capacity = ( old_capacity == 0 ? 3 : old_capacity * 2 + 1 );
    T *keys = new T[ this->capacity ];
    V *values = new V[ this->capacity ];
    for ( size_t i = 0; i < old_capacity; i++ ) {
      if ( this->keys[ i ] == 0 )
        continue;
      size_t j = this->keys[ i ].hash() % this->capacity;
      while ( keys[ j ] != 0 )
        j = ( j + 1 ) % this->capacity;
      keys[ j ] = this->keys[ i ];
      values[ j ] = this->values[ i ];
    }
    if ( old_capacity > 0 ) {
      delete [] this->keys;
      delete [] this->values;
    }
    this->keys = keys;
    this->values = values;
  }
  size_t count( void ) const {
    return this->used;
  }
};

template <class T, class V> struct X_HashIterator {
  X_Hash<T, V> &hash;
  size_t i;
  X_HashIterator( X_Hash<T, V> &h) : hash( h ), i( 0 ) {}
  bool operator++( void ) {
    for (;;) {
      if ( ++this->i >= this->hash.capacity + 1 )
        return false;
      if ( this->hash.keys[ this->i - 1 ] != 0 )
        return true;
    }
  }
  const T & key( void ) const {
    return this->hash.keys[ this->i - 1 ];
  }
  V & value( void ) const {
    return this->hash.values[ this->i - 1 ];
  }
};

template <class V> struct X_Array : public X_Hash<V, size_t> {
  size_t * index;
  X_Array() : X_Hash<V, size_t>(), index( 0 ) {}
  ~X_Array() {
    if ( this->capacity > 0 )
      ::free( this->index );
  }
  V & operator[]( size_t i ) {
    return this->keys[ this->index[ i ] ];
  }
  void addUnique( const V &value ) {
    size_t pos;
    for (;;) {
      if ( this->find( value, pos ) )
        return;
      if ( this->used < this->capacity / 2 ) {
        this->index[ this->used ] = pos;
        this->keys[ pos ] = value;
        this->values[ pos ] = this->used++;
        return;
      }
      this->grow();
      this->index = (size_t *)
        ::realloc( this->index, this->capacity * sizeof( size_t ) );
      for ( size_t i = 0; i < this->capacity; i++ ) {
        if ( this->keys[ i ] != 0 )
          this->index[ this->values[ i ] ] = i;
      }
    }
  }
};
