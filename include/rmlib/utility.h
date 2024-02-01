/*****************************************************************************\
*  Copyright (c) 2023 Ricardo Machado, Sydney, Australia All rights reserved.
*
*  MIT License
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to
*  deal in the Software without restriction, including without limitation the
*  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*  sell copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in
*  all copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*  IN THE SOFTWARE.
*
*  You should have received a copy of the MIT License along with this program.
*  If not, see https://opensource.org/licenses/MIT.
\*****************************************************************************/
#pragma once

#include <concepts>

namespace rmlib {

   // define a concept to allow containers that have data() and size() methods such as 
   // std::span, std::string_view, std::array, std::string and std::vector. Also the
   // container type has to be 1 byte size, such as char, unsigned char and char8_t
   template<typename T>
   concept DataSizeContainer = requires(T a) 
   {
      { a.data() } -> std::same_as<typename T::value_type*>;
      { a.size() } -> std::same_as<std::size_t>;
   } && sizeof(typename T::value_type) == 1;

   // define a concept to allow containers that have data(), size(), resize() and
   // clear() methods such as std::string and std::vector. Also the container type  
   // has to be 1 byte size, such as char, unsigned char and char8_
   template<typename T>
   concept DataSizeResizeContainer = requires(T a, std::size_t n) 
   {
      { a.data() } -> std::same_as<typename T::value_type*>;
      { a.size() } -> std::same_as<std::size_t>;
      { a.resize(n) } -> std::same_as<void>;
      { a.clear() } -> std::same_as<void>;
   } && sizeof(typename T::value_type) == 1;
   
} // namespace rmlib
