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

#define _FILE_OFFSET_BITS 64
#include <stdio.h>

#include "rmlib/xplat.h"
#include "rmlib/status.h"
#include "rmlib/utility.h"


#if defined(XPLAT_OS_WINDOWS)
   #include <windows.h>
   #include <io.h>
   using off64_t = long long;

   inline bool file_exists(const char* filepath) noexcept
   {
      if (!filepath) return false;
      DWORD fileAttributes = GetFileAttributes(filepath);
      return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
   }
#else
   #include <unistd.h>
   #include <sys/types.h>
   #define _ftelli64 ftell
   #define _fseeki64 fseek
   #define remove(s) unlink(s)

   inline errno_t fopen_s(FILE** file, const char* filename, const char* mode) noexcept
   {
      *file = fopen(filename, mode);
      return *file ? 0 : errno;
   }

   inline bool file_exists(const char* filepath) noexcept
   {
      if (!filepath) return false;)
      return access(filepath, F_OK) != -1;
   }
#endif

namespace rmlib {

   enum class open_access_t : unsigned
   {
        read = 0           // only read operations allowed
      , write              // only write operations allowed 
      , read_write         // read and write operations allowed
      , append             // write operations are appended to the end of the file
   };

   enum class open_mode_t : unsigned
   {
        open_existing = 0  // file must exist
      , create_new         // file must not exist
      , create_always      // file is created if new or truncated if it exsits
   };

   enum class seek_mode_t : int {begin = SEEK_SET, current = SEEK_CUR, end = SEEK_END };

   class fstream_t
   {
      FILE* handle_{};

   public:
      using file_handle_t = FILE*;

      fstream_t() = default;
      fstream_t(const fstream_t&) = delete;
      fstream_t(fstream_t&&) = default;
      fstream_t& operator=(const fstream_t&) = delete;
      fstream_t& operator=(fstream_t&&) = default;

      ~fstream_t() noexcept
      {
         close();
      }

      file_handle_t handle() const noexcept
      {
         return handle_;
      }

      bool is_eof() const noexcept
      {
         if (!handle_) return true;
         return feof(handle_);
      }

      // status_t is set to EINVAL if access more read is used with create_new or create_always
      status_t open(const char* filename, open_access_t access, open_mode_t mode) noexcept
      {
         const char* mode_str[4][3] =
         {
            /*                  open_existing  create_new  create_always */
            /* read        */ {   "rb",          "?",         "?"       },
            /* write       */ {   "wxb",         "wxb",       "wb"      },
            /* read_write  */ {   "r+b",         "wx+b",      "w+n"     },
            /* append      */ {   "a+xb",        "a+xb",      "a+b"     }
         };
         if (*mode_str[static_cast<unsigned>(access)][static_cast<unsigned>(mode)] == '?') return status_t(EINVAL);
         close();
         status_t status = fopen_s(&handle_, filename, mode_str[static_cast<unsigned>(access)][static_cast<unsigned>(mode)]);
         if (status.nok())
         {
            handle_ = nullptr;
         }
         return status;
      }

      status_t open(const std::string& filename, open_access_t access, open_mode_t mode) noexcept
      {
         return open(filename.c_str(), access, mode);
      }

      status_t close() noexcept
      {
         if (handle_)
         {
            fflush(handle_);
            int ret = fclose(handle_);
            handle_ = nullptr;
            if (ret) return status_t(errno);
         }
         return status_t{};
      }

      status_t read(void* buffer, size_t size, size_t& bytes_read) noexcept
      {
         status_t status;
         if (handle_ == nullptr) return status_t(EBADF);
         bytes_read = fread(buffer, 1, size, handle_); 
         if (ferror(handle_)) return status.reset(errno);
         return status;
      }

      template <DataSizeResizeContainer T>
      status_t read(T& buffer, size_t size, size_t& bytes_read) noexcept
      {
         status_t status;
         size_t bytes_read;
         if (handle_ == nullptr) return status.reset(EBADF);
         buffer.resize(size);
         status = read(buffer.data(), buffer.size(), bytes_read);
         if (status.ok())
         {
            buffer.resize(bytes_read);
         }
         return status;
      }

      status_t write(const void* buffer, size_t size, size_t& bytes_written) noexcept
      {
         if (handle_ == nullptr) return status_t(EBADF);
         bytes_written = fwrite(buffer, 1, size, handle_);
         if (ferror(handle_)) return status_t(errno);
         return status_t{};
      }

      status_t write(const char* buffer, size_t& bytes_written) noexcept
      {
         if (!buffer) return status_t(EINVAL);  
         return write(buffer, strlen(buffer), bytes_written);
      }

      template <DataSizeContainer T>
      status_t write(const T& buffer, size_t& bytes_written) noexcept
      {
         return write(buffer.data(), buffer.size(), bytes_written);
      }

      void flush() noexcept
      {
         if (handle_) fflush(handle_);
      }

      // return file size in bytes
      size_t size() noexcept
      {
         off64_t size{};
         if (handle_)
         {
            off64_t current = _ftelli64(handle_);
            _fseeki64(handle_, 0, SEEK_END);
            size = _ftelli64(handle_);
            _fseeki64(handle_, current, SEEK_SET);
         }
         return size;
      }

      off64_t tell() noexcept
      {
         if (handle_) return _ftelli64(handle_);
         return -1;
      }

      status_t seek(off64_t offset, seek_mode_t mode) noexcept
      {
         if (handle_) return status_t(_fseeki64(handle_, offset, static_cast<int>(mode)));
         return status_t(EBADF);
      }

      status_t rewind() noexcept
      {
         return seek(0, seek_mode_t::begin);
      }

      // check if file exists.
      bool exists(const char* filename) noexcept
      {
         return file_exists(filename);
      }

      bool exists(const std::string& filename) noexcept
      {
         return exists(filename.c_str());
      }

      status_t remove(const char* filename) noexcept
      {
         return status_t(remove(filename));
      }

      status_t remove(const std::string& filename) noexcept
      {
         return remove(filename.c_str());
      }

   }; // class fstream_t
      
} // namespace rmlib::time
