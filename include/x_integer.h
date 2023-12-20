
struct X_Integer {
  long long v;
  X_Integer() : v( 0 ) {}
  X_Integer &operator=( long long rhs ) {
    v = rhs;
    return *this;
  }
  X_Integer &operator+=( long long rhs ) {
    v += rhs;
    return *this;
  }
  operator long long() const { return v; }
};

