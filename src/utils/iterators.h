#pragma once

#include <iterator>
#include <cassert>
#include <cstddef>

using std::ptrdiff_t;

// Random-access iterator wrapping any raw data, to be used for custom container classes
template <typename T>
class RawIterator {
 public:
  // public typedefs for STL algorithms
  using value_type        = T;
  using reference         = T&;
  using difference_type   = ptrdiff_t;
  using pointer           = T*;
  using iterator_category = std::random_access_iterator_tag;
  using self              = RawIterator<T>;

  // constructors
  explicit RawIterator(pointer ptr = nullptr) : m_ptr(ptr) {}

  RawIterator(const self& rawIterator) = default;

  ~RawIterator() = default;

  // operators
  self& operator=(const self& rawIterator) = default;

  self& operator=(pointer ptr) {
    m_ptr = ptr;
    return *this;
  }

  explicit operator bool() const { return m_ptr != nullptr; }

  self& operator++() {
    ++m_ptr;
    return *this;
  }
  self operator++(int) {
    self tmp = *this;
    ++m_ptr;
    return tmp;
  }
  self& operator+=(const ptrdiff_t& x) {
    m_ptr += x;
    return *this;
  }
  self& operator--() {
    --m_ptr;
    return *this;
  }
  self operator--(int) {
    self tmp = *this;
    --m_ptr;
    return tmp;
  }
  self& operator-=(const ptrdiff_t& x) {
    m_ptr -= x;
    return *this;
  }

  reference operator[](const difference_type n) { return m_ptr[n]; }
  reference operator*() const { return *m_ptr; }
  pointer operator->() { return m_ptr; }

  // friend operators
  friend bool operator==(const self& x, const self& y) { return x.m_ptr == y.m_ptr; }
  friend bool operator!=(const self& x, const self& y) { return x.m_ptr != y.m_ptr; }

  friend bool operator<(const self& x, const self& y) { return x.m_ptr < y.m_ptr; }
  friend bool operator<=(const self& x, const self& y) { return x.m_ptr <= y.m_ptr; }
  friend bool operator>(const self& x, const self& y) { return x.m_ptr > y.m_ptr; }
  friend bool operator>=(const self& x, const self& y) { return x.m_ptr >= y.m_ptr; }

  friend difference_type operator-(const self& x, const self& y) {
    return std::distance(y.m_ptr, x.m_ptr);
  }

  friend self operator+(const self& x, difference_type y) { return self(x.m_ptr + y); }

  friend self operator+(difference_type x, const self& y) { return y + x; }

  friend self operator-(const self& x, difference_type y) { return self(x.m_ptr - y); }

 protected:
  T* m_ptr;
};

// Random-access iterator wrapping any raw data, to be used for custom container classes
template <typename T>
class ConstRawIterator {
 public:
  // public typedefs for STL algorithms
  using value_type        = T;
  using reference         = T&;
  using difference_type   = ptrdiff_t;
  using pointer           = T*;
  using iterator_category = std::random_access_iterator_tag;
  using self              = ConstRawIterator<T>;

  // constructors
  explicit ConstRawIterator(pointer ptr = nullptr) : m_ptr(ptr) {}

  ConstRawIterator(const self& rawIterator) = default;

  ~ConstRawIterator() = default;

  // operators
  self& operator=(const self& rawIterator) = default;

  self& operator=(pointer ptr) {
    m_ptr = ptr;
    return *this;
  }

  explicit operator bool() const { return m_ptr != nullptr; }

  self& operator++() {
    ++m_ptr;
    return *this;
  }
  self operator++(int) {
    self tmp = *this;
    ++m_ptr;
    return tmp;
  }
  self& operator+=(const ptrdiff_t& x) {
    m_ptr += x;
    return *this;
  }
  self& operator--() {
    --m_ptr;
    return *this;
  }
  self operator--(int) {
    self tmp = *this;
    --m_ptr;
    return tmp;
  }
  self& operator-=(const ptrdiff_t& x) {
    m_ptr -= x;
    return *this;
  }

  value_type operator[](const difference_type n) const { return m_ptr[n]; }
  value_type operator*() const { return *m_ptr; }
  pointer operator->() { return m_ptr; }

  // friend operators
  friend bool operator==(const self& x, const self& y) { return x.m_ptr == y.m_ptr; }
  friend bool operator!=(const self& x, const self& y) { return x.m_ptr != y.m_ptr; }

  friend bool operator<(const self& x, const self& y) { return x.m_ptr < y.m_ptr; }
  friend bool operator<=(const self& x, const self& y) { return x.m_ptr <= y.m_ptr; }
  friend bool operator>(const self& x, const self& y) { return x.m_ptr > y.m_ptr; }
  friend bool operator>=(const self& x, const self& y) { return x.m_ptr >= y.m_ptr; }

  friend difference_type operator-(const self& x, const self& y) {
    return std::distance(y.m_ptr, x.m_ptr);
  }

  friend self operator+(const self& x, difference_type y) { return self(x.m_ptr + y); }

  friend self operator+(difference_type x, const self& y) { return y + x; }

  friend self operator-(const self& x, difference_type y) { return self(x.m_ptr - y); }

 protected:
  T* m_ptr;
};

template <class itr_t>
class StrideIterator {
 public:
  // public typedefs for STL algorithms
  using value_type        = typename std::iterator_traits<itr_t>::value_type;
  using reference         = typename std::iterator_traits<itr_t>::reference;
  using difference_type   = typename std::iterator_traits<itr_t>::difference_type;
  using pointer           = typename std::iterator_traits<itr_t>::pointer;
  using iterator_category = std::random_access_iterator_tag;
  using self              = StrideIterator;

  // constructors
  StrideIterator() : m(nullptr), step(0){};
  StrideIterator(const self& x) : m(x.m), step(x.step) {}
  StrideIterator(itr_t x, difference_type n) : m(x), step(n) { assert(n != 0); }

  // operators
  self& operator++() {
    m += step;
    return *this;
  }
  self operator++(int) {
    self tmp = *this;
    m += step;
    return tmp;
  }
  self& operator+=(const difference_type x) {
    m += (x * step);
    return *this;
  }
  self& operator--() {
    m -= step;
    return *this;
  }
  self operator--(int) {
    self tmp = *this;
    m -= step;
    return tmp;
  }
  self& operator-=(const difference_type x) {
    m -= x * step;
    return *this;
  }
  reference operator[](const difference_type n) { return m[n * step]; }
  reference operator*() const { return *m; }

  // friend operators
  friend bool operator==(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m == y.m;
  }
  friend bool operator!=(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m != y.m;
  }
  friend bool operator<(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m < y.m;
  }
  friend bool operator<=(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m <= y.m;
  }
  friend bool operator>(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m > y.m;
  }
  friend bool operator>=(const self& x, const self& y) {
    assert(x.step == y.step);
    return x.m >= y.m;
  }
  friend difference_type operator-(const self& x, const self& y) {
    assert(x.step == y.step);
    return (x.m - y.m) / x.step;
  }

  friend self operator+(const self& x, difference_type y) {
    return self(x.m + (y * x.step), x.step);
  }

  friend self operator+(difference_type x, const self& y) { return y + x; }

  friend self operator-(const self& x, difference_type y) {
    return self(x.m - (y * x.step), x.step);
  }

 private:
  itr_t m;
  difference_type step;
};
