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

#include <cstdio>
#include <cerrno>
#include <cstring>

#include "rmlib/rmxplat.h"
#include "rmlib/rmstatus.h"

namespace rmlib {

   namespace file {
      
      enum class open_mode_t : unsigned { open = 0, create, overwrite };      
      enum class open_type_t : unsigned { read = 0, write, read_write, append };
      enum class open_flag_t { text, binary };
      enum class seek_mode_t { begin, current, end };
      enum class lock_type_t { shared, exclusive, unlock };

      using status_t = rmlib::status_t<>;
      using offset_t = off64_t;
               
      namespace xplat {
         
         
         status_t lock(FILE* fd, lock_type_t type, offset_t 
         
      } // namespace xplat
      
      class file_t
      {
         FILE* fd_{};
         std::string path_;
            
      public:
         file_t() = default;
         file_t(const file_t&) = delete;
         file_t(file_t&&) noexcept = default;
         file_t& operator=(const file_t&) = delete;
         file_t& operator=(file_t&&) noexcept = default;
         
         ~file_t() noexcept
         {
            close();
         }
         
         bool is_open() const noexcept
         {
            return fd_ != nullptr;
         }
      
         std::string path() const noexcept
         {
            return path_;
         }
      
         FILE* handle() noexcept
         {
            return fd_;
         }
         
         const FILE* handle() const noexcept
         {
            return fd_;
         }
      
         status_t open(const std::string& path, open_mode_t mode = open_mode_t::open, open_type_t type = open_type_t::read_write, open_flag_t flag = open_flag_t::text) noexcept
         {
            close();
            if (fd_ = fopen(path.c_str(), xlate(mode, type, flag).c_str()); !fd_)
            {
               return status_t(-1);
            }
            path_ = path;
            return status_t{};
         }
      
         status_t close() noexcept
         {
            status_t status;
            if (is_open())
            {
               status = fclose(fd_);
               fd_ = nullptr;
            }
            return status;
         }
      
         status_t write(const char* buffer, size_t size) noexcept
         {
            if (!is_open() || !buffer) return status_t{ EINVAL };
            if (!size) return status_t{};
            clearerr(fd_);
            fwrite(buffer, sizeof(char), size, fd_);
            return status_t{ ferror(fd_) };
         }
      
         status_t write(const char* text) noexcept
         {
            if (!text) return status_t{ EINVAL };
            return write(text, strlen(text));
         }
      
         template <DataSizeContainer T>
         status_t write(const T& buffer) noexcept
         {
            return write(buffer.data(), buffer.size());
         }
            
         size_t size() const noexcept
         {
            if (!is_open()) return 0;
            clearerr(fd_);
            long pos = ftell(fd_);
            if (pos < 0l) return 0;
            return static_cast<size_t>(pos);
         }
      
         status_t flush() noexcept
         {
            if (!is_open()) return status_t{ EINVAL };
            clearerr(fd_);
            if (fflush(fd_) != 0) return status_t{ ferror(fd_) };
            return status_t{};
         }
      
         status_t seek(offset_t offset, seek_mode_t mode = seek_mode_t::begin) noexcept
         {
            using enum seek_mode_t;
            int origin = (mode == begin ? SEEK_SET : (mode == end ? SEEK_END : SEEK_CUR));
            if (int ret = _fseeki64(fd_, offset, origin); ret != 0)
            {
               return status_t(-1);
            }
            return status_t{};
         }
         
         offset_t tell() const noexcept
         {
            return _ftelli64(fd_);
         }
         
         status_t lock(offset_t offset, size_t bytes, lock_mode_t mode) noexcept
         {
            
         }
         
         status_t try_lock(offset_t offset, size_t bytes, lock_mode_t mode) noexcept
         {
         }
         
         status_t unlock(offset_t offset, size_t bytes) noexcept
         {
         }
         
      private:
         std::string xlate(mode_t mode, type_t type, flag_t flag) const noexcept
         {
            const char* matrix[][] = 
            {
               /*                read,    write,   read_write, append */
               /* open */      { "r",     "wx",    "r+",       "ax" },
               /* create */    { "w+x",   "w+x",    "w+x",     "a+x" },
               /* overwrite */ { "w+",    "w+",    "w+",       "a+" }
            };
            std::string param{ matrix[static_cast<unsigned>(mode)][static_cast<unsigned>(type)] }; 
            if (flag == flag_t::binary)
            {
               param += "b";
            }
            return param;
         }      
      }; // class file_t
   
   } // namespace file

} // namespace rmlib
