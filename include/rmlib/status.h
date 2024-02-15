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

#include "rmlib/xplat.h"
#include "rmlib/utility.h"

namespace rmlib {

   constexpr size_t MAX_REASON_MESSAGE_SIZE = 256;

   template <typename T, T OK, T NOK, T(*LAST_ERROR)()>
   class status_base_t
   {
      T errno_{};
      std::string reason_{};

   public:
      using error_t = T;

      status_base_t() = default;
      status_base_t(const status_base_t&) = default;
      status_base_t(status_base_t&&) noexcept = default;
      status_base_t& operator=(const status_base_t&) = default;
      status_base_t& operator=(status_base_t&&) noexcept = default;

      status_base_t(error_t err) noexcept
         : errno_{ err == NOK ? last_error() : err }
      {}

      status_base_t(error_t err, const std::string& reason) noexcept
         : errno_{ err == NOK ? last_error() : err }
         , reason_{ reason }
      {}

      status_base_t& operator=(error_t err) noexcept
      {
         this->errno_ = (err == NOK) ? last_error() : err;
         reason_.clear();
         return *this;
      }

      [[nodiscard]]
      bool ok() const noexcept
      {
         return this->errno_ == OK;
      }

      [[nodiscard]]
      bool nok() const noexcept
      {
         return !ok();
      }

      error_t error() const noexcept
      {
         return this->errno_;
      }

      void clear() noexcept
      {
         this->errno_ = OK;
      }

      status_base_t& reset(error_t err) noexcept
      {
         this->errno_ = (err == NOK) ? last_error() : err;
         reason_.clear();
         return *this;
      }

      status_base_t& reset(error_t err, const std::string& reason) noexcept
      {
         this->errno_ = (err == NOK) ? last_error() : err;
         reason_ = reason;
         return *this;
      }

      error_t last_error() const noexcept
      {
         return LAST_ERROR();
      }

      [[nodiscard]]
      virtual std::string reason() const noexcept
      {
         if (this->errno_ != OK)
         {
            if (!reason_.empty()) return reason_;
            std::array<char, MAX_REASON_MESSAGE_SIZE> buffer{};
            strerror_s(buffer.data(), buffer.size(), this->errno_);
            return buffer.data();
         }
         return "No errors detected";
      }
   }; // class status_base_t

   using status_t = status_base_t<int, 0, -1, []() { return errno; } > ;

} // namespace rmlib