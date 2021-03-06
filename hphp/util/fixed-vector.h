/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2016 Facebook, Inc. (http://www.facebook.com)     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/
#ifndef incl_HPHP_UTIL_FIXED_VECTOR_H_
#define incl_HPHP_UTIL_FIXED_VECTOR_H_

#include <algorithm>
#include <vector>
#include <stdexcept>
#include "hphp/util/assertions.h"
#include "hphp/util/compact-tagged-ptrs.h"

namespace HPHP {

//////////////////////////////////////////////////////////////////////

/*
 * Fixed size vector with a maximum allocation of 2^16 * sizeof(T).
 *
 * Useful when you know the exact size something will take, and don't
 * need more than that many elements.
 */
template<class T>
struct FixedVector {
  typedef uint32_t size_type;
  typedef T value_type;
  typedef T* iterator;
  typedef const T* const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  /*
   * Default constructor leaves a FixedVector with size() == 0.
   */
  explicit FixedVector() {}

  FixedVector(const FixedVector& fv) = delete;
  FixedVector& operator=(const FixedVector&) = delete;

  /*
   * Create a FixedVector using the supplied std::vector as a starting
   * point.  Throws if the sourceVec is too large.
   */
  explicit FixedVector(const std::vector<T>& sourceVec) {
    move(sourceVec);
  }

  FixedVector(FixedVector<T>&& fv) {
    swap(fv);
  }

  ~FixedVector() {
    T* p = m_sp.ptr();
    for (uint32_t i = 0, sz = size(); i < sz; ++i) {
      p[i].~T();
    }
    free(p);
  }

  /*
   * Assign this FixedVector to contain values from a std::vector.
   * Destroys contents this one previously had, if any.
   */
  FixedVector& operator=(const std::vector<T>& sourceVec) {
    FixedVector newOne(sourceVec);
    swap(newOne);
    return *this;
  }

  FixedVector& operator=(std::vector<T>&& src) {
    move(src);
    return *this;
  }

  uint32_t size()  const { return m_sp.size(); }
  bool     empty() const { return !size(); }

  const T& operator[](uint32_t idx) const { return m_sp.ptr()[idx]; }
        T& operator[](uint32_t idx)       { return m_sp.ptr()[idx]; }

  const_iterator begin() const { return m_sp.ptr(); }
  const_iterator end()   const { return m_sp.ptr() + size(); }
  iterator begin()             { return m_sp.ptr(); }
  iterator end()               { return m_sp.ptr() + size(); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend()   { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  void swap(FixedVector& fv) {
    std::swap(m_sp, fv.m_sp);
  }

private:
  template<class Src>
  void move(Src& sourceVec) {
    auto const neededSize = sourceVec.size();

    if (neededSize >> 16) {
      throw std::runtime_error("FixedVector maximum size exceeded");
    }

    auto const ptr = neededSize > 0
      ? static_cast<T*>(malloc(neededSize * sizeof(T)))
      : nullptr;

    size_t i = 0;
    try {
      for (; i < neededSize; ++i) {
        new (&ptr[i]) T(std::move(sourceVec[i]));
      }
    } catch (...) {
      for (size_t j = 0; j < i; ++j) {
        ptr[j].~T();
      }
      free(ptr);
      throw;
    }
    assert(i == neededSize);
    m_sp.set(neededSize, ptr);
  }

private:
  CompactSizedPtr<T> m_sp;
};

//////////////////////////////////////////////////////////////////////

static_assert(sizeof(FixedVector<int>) == sizeof(CompactSizedPtr<int>),
              "Keeping this thing small is most of the point");

//////////////////////////////////////////////////////////////////////

}

#endif
