/*****************************************************************************

Copyright (c) 2023, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

#pragma once
/* A normally small vector, inspired by llvm::SmallVector */
#include "my_global.h"
#include <iterator>
#include <memory>

class small_vector_base
{
protected:
  typedef uint32_t Size_T;
  void *BeginX;
  Size_T Size= 0;
  small_vector_base()= delete;
  small_vector_base(void *small) : BeginX(small) {}
  ATTRIBUTE_COLD
  void grow_by_1(void *small, size_t element_size) noexcept;
public:
  size_t size() const { return Size; }
  bool empty() const { return !Size; }
  void clear() { Size= 0; }
protected:
  void set_size(size_t N) { Size= Size_T(N); }
};

template <typename T, unsigned N>
class small_vector : public small_vector_base
{
  /** The fixed storage allocation */
  T small[N];

  using small_vector_base::set_size;

  void grow_if_needed() noexcept
  {
    if (unlikely(size() >= N))
      grow_by_1(small, sizeof *small);
  }

public:
  small_vector() : small_vector_base(small)
  {
    TRASH_ALLOC(small, sizeof small);
  }
  ~small_vector() noexcept
  {
    if (small != begin())
      my_free(begin());
    MEM_MAKE_ADDRESSABLE(small, sizeof small);
  }

  void fake_defined() const noexcept
  {
    ut_ad(empty());
    ut_ad(is_small());
    MEM_MAKE_DEFINED(small, sizeof small);
  }
  void make_undefined() const noexcept { MEM_UNDEFINED(small, sizeof small); }

  bool is_small() const noexcept { return small == BeginX; }

  void deep_clear() noexcept
  {
    if (!is_small())
    {
      my_free(BeginX);
      BeginX= small;
    }
    set_size(0);
  }

  using iterator= T *;
  using const_iterator= const T *;
  using reverse_iterator= std::reverse_iterator<iterator>;
  using reference= T &;
  using const_reference= const T&;

  iterator begin() { return static_cast<iterator>(BeginX); }
  const_iterator begin() const { return static_cast<const_iterator>(BeginX); }
  iterator end() { return begin() + size(); }
  const_iterator end() const { return begin() + size(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

  reference operator[](size_t i) { assert(i < size()); return begin()[i]; }
  const_reference operator[](size_t i) const
  { return const_cast<small_vector&>(*this)[i]; }

  void erase(const_iterator S, const_iterator E)
  {
    set_size(std::move(const_cast<iterator>(E), end(),
                       const_cast<iterator>(S)) - begin());
  }

  void emplace_back(T &&arg)
  {
    grow_if_needed();
    ::new (end()) T(arg);
    set_size(size() + 1);
  }
};
