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

namespace rmlib::status {
   
   constexpr size_t MAX_ERROR_STRING = 256;
}

#if defined(XPLAT_OS_WIN)
   #include <windows.h>

   using uerrno_t = unsigned int;
   using ierrno_t = errno_t;

   inline std::string GetErrorMessage(uerrno_t err) noexcept
   {
      std::string str;
      const DWORD MAX_ERROR_STRING = 1024; 
      std::array<char, MAX_ERROR_STRING> errorMessageBuffer;
      DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, errorMessageBuffer.data(), MAX_ERROR_STRING, nullptr);
      if (len > 0)
      {
         str = errorMessageBuffer.data();
      }
      else
      {
         str = "Failed to retrieve error message. Error ";
         str += std::to_string(err);
      }
      return str;
   }

   inline std::string GetErrorMessage(ierrno_t err) noexcept
   {
      const unsigned MAX_ERROR_STRING = 1024;
      std::array<char, MAX_ERROR_STRING> text;
      strerror_s(text.data(), text.size(), err);
      return std::string(text.data());

   }

   inline std::string GetErrorMessage(DWORD err) noexcept
   {
      std::string str;
      const DWORD MAX_ERROR_STRING = 1024;
      std::array<char, MAX_ERROR_STRING> errorMessageBuffer;
      DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, 0, errorMessageBuffer.data(), MAX_ERROR_STRING, nullptr);
      if (len > 0)
      {
         str = errorMessageBuffer.data();
      }
      else
      {
         str = "Failed to retrieve error message. Error ";
         str += std::to_string(err);
      }
      return str;
   }

   inline std::string GetLastErrorMessage() noexcept
   {
      return GetErrorMessage(GetLastError());
   }
#else
   #if !defined(errno_t)
      using errno_t = int;
   #endif

   inline errno_t GetLastError() noexcept
   {
      return errno;
   }

   inline std::string GetErrorMessage(errno_t err) noexcept
   {
      return std::string(strerror(err));
   }

   inline std::string GetLastErrorMessage() noexcept
   {
      return GetErrorMessage(GetLastError());
   }

#endif

namespace rmlib {
   
   namespace status {
      
      constexpr int OK = 0;
      constexpr int NOTOK = -1;
   
      typedef int (*STATUS_FUNC)();

      inline int get_last_error() noexcept
      {
         return GetLastError();
      }
      
   } // namespace status
   
   template <typename T = int, T OK = status::OK, T NOK = status::NOTOK, status::STATUS_FUNC FUNC = status::get_last_error>
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
         : errno_{ err == NOK ? FUNC() : err }
      {}

      status_base_t(error_t err, const std::string& reason) noexcept
         : errno_{ err == NOK ? FUNC() : err }
         , reason_{ reason }
      {}

      status_base_t& operator=(error_t err) noexcept
      {
         this->errno_ = (err == NOK) ? FUNC() : err;
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
         this->errno_ = (err == NOK) ? FUNC() : err;
         reason_.clear();
         return *this;
      }

      status_base_t& reset(error_t err, const std::string& reason) noexcept
      {
         this->errno_ = (err == NOK) ? FUNC() : err;
         reason_ = reason;
         return *this;
      }

      [[nodiscard]]
      virtual std::string reason() const noexcept
      {
         if (this->errno_ != OK)
         {
            if (!reason_.empty()) return reason_;
            return GetErrorMessage(this->errno_);
         }
         return "No errors detected";
      }
   }; // class status_base_t
   
   using status_t = status_base_t<>;

} // namespace rmlib