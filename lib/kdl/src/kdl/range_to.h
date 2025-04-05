/*
 Copyright 2024 Kristian Duske

 Permission is hereby granted, free of charge, to any person obtaining a copy of this
 software and associated documentation files (the "Software"), to deal in the Software
 without restriction, including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "kdl/range_utils.h"

#include <ranges>

namespace kdl
{
namespace detail
{

// Type acts as a tag to find the correct operator| overload
template <typename C>
struct to_container_helper
{
};

// This actually does the work
template <typename Container, std::ranges::range R>
  requires std::
    convertible_to<std::ranges::range_value_t<R>, typename Container::value_type>
  Container operator|(R&& r, to_container_helper<Container>)
{
  return Container{get_begin(r), get_end(r)};
}

} // namespace detail

template <std::ranges::range Container>
  requires(!std::ranges::view<Container>)
auto to()
{
  return detail::to_container_helper<Container>{};
}

} // namespace kdl
