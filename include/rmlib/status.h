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

#include <cerrno>
#include <cstring>
#include <string>
#include <array>
#include <format>

namespace rmlib {
   
   namespace status {
      
      constexpr size_t MAX_ERROR_STRING = 256;
      constexpr int OK = 0;
      constexpr int NOTOK = -1;
   
      typedef int (*STATUS_FUNC)();
      
      inline int last_error()
      {
         return errno;
      }
      
   } // namespace status
   
   template <typename T = int, T OK = status::OK, T NOK = status::NOTOK, status::STATUS_FUNC FUNC = status::last_error>
   class status_t
   {
      T errno_{};

   public:
      using error_t = T;
      
      status_t() = default;
      status_t(const status_t&) = default;
      status_t(status_t&&) noexcept = default;
      status_t& operator=(const status_t&) = default;
      status_t& operator=(status_t&&) noexcept = default;

      virtual ~status_t() noexcept
      {}

      status_t(error_t err) noexcept
         : errno_{ err == NOK ? FUNC() : err }
      {}

      status_t& operator=(error_t err) noexcept
      {
         errno_ = (err == NOK) ? FUNC() : err;
         return *this;
      }

      bool ok() const noexcept
      {
         return errno_ == OK;
      }

      bool nok() const noexcept
      {
         return !ok();
      }

      error_t error() const noexcept
      {
         return errno_;
      }

      void clear() noexcept
      {
         errno_ = OK;
      }

      virtual std::string reason() const noexcept
      {
         if (errno_ != OK)
         {
            std::array<char, status::MAX_ERROR_STRING> text;
            strerror_s(text.data(), text.size(), errno_);
            return std::string(text.data());
         }
         return "No errors detected";
      }
   }; // class status_t
   
} // namespace rmlib