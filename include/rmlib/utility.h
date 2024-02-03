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
#include <thread>
#include <atomic>

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
   
   inline uint32_t low32(uint64_t value) noexcept
   {
      return static_cast<uint32_t>(value & 0xffffffffull);
   }

   inline uint32_t high32(uint64_t value) noexcept
   {
      return static_cast<uint32_t>(value  >> 32);
   }

   inline uint64_t make64(uint32_t high, uint32_t low) noexcept
   {
      uint64_t hv{ high };
      return static_cast<uint64_t>((hv << 32) | low);
   }

   class spin_lock_t 
   {
      std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
   
   public:
      spin_lock_t() = default;
      spin_lock_t(const spin_lock_t&) = delete;
      spin_lock_t(spin_lock_t&&) noexcept = default;
      spin_lock_t& operator=(const spin_lock_t&) = delete;
      spin_lock_t& operator=(spin_lock_t&&) noexcept = default;

      void lock() noexcept
      {
         while (flag_.test_and_set(std::memory_order_acquire)) 
         {
            std::this_thread::yield(); // Spin until the lock is acquired
         }
      }

      void unlock() noexcept
      {
         flag_.clear(std::memory_order_release);
      }
   };

   class spin_guard_t
   {
      spin_lock_t& lock_;

   public:
      spin_guard_t() = delete;
      spin_guard_t(const spin_guard_t&) = delete;
      spin_guard_t(spin_guard_t&&) noexcept = delete;
      spin_guard_t& operator=(const spin_guard_t&) = delete;
      spin_guard_t& operator=(spin_guard_t&&) noexcept = delete;

      spin_guard_t(spin_lock_t& spl) noexcept
         : lock_{ spl }
      {
         lock_.lock();
      }

      ~spin_guard_t() noexcept
      {
         lock_.unlock();
      }
   };

} // namespace rmlib
