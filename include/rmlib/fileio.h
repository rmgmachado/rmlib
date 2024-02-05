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
#include <string>

#include "rmlib/xplat.h"
#include "rmlib/status.h"
#include "rmlib/utility.h"

constexpr size_t MAX_ERROR_MESSAGE_SIZE = 256;

#if defined(XPLAT_OS_WIN)

#include <windows.h>

   using offset_t = uint64_t;

#else

   #if defined(XPLAT_OS_LINUX)
      #define _FILE_OFFSET_BITS 64
   #endif
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <unistd.h>

   using offset_t = off_t;
   using errno_t = int;

#endif

   namespace rmlib {

      enum class open_mode_t { open_existing = 0, create_new, create_always };
      enum class open_type_t { read = 0, write, read_write, append };
      enum class seek_mode_t { begin, current, end };
      enum class lock_type_t { shared, exclusive, unlock };

      inline std::string GetErrorMessage(errno_t err) noexcept;
      inline std::string GetLastErrorMessage() noexcept;

      namespace fileio {

         inline DWORD xlate_open_access(open_type_t type) noexcept;
         DWORD xlate_open_create(open_mode_t mode) noexcept;
         inline DWORD xlate_seek_mode(seek_mode_t mode) noexcept;
      };

      class file_t
      {
         HANDLE fd_{ INVALID_HANDLE_VALUE };
         open_mode_t mode_{ open_mode_t::open_existing };
         open_type_t type_{ open_type_t::read_write };
         std::string path_;

      public:
         using handle_t = HANDLE;

         file_t() = default;
         file_t(const file_t&) = delete;
         file_t(file_t&&) noexcept = default;
         file_t& operator=(const file_t&) = delete;
         file_t& operator=(file_t&&) noexcept = default;

         ~file_t() noexcept
         {
            close();
         }

         [[nodiscard]]
         bool is_open() const noexcept
         {
            return fd_ != INVALID_HANDLE_VALUE;
         }

         [[nodiscard]]
         std::string path() const noexcept
         {
            return path_;
         }

         handle_t handle() noexcept
         {
            return fd_;
         }

         [[nodiscard]]
         handle_t handle() const noexcept
         {
            return fd_;
         }

         // opening a file with mode == create_new and type == read will not 
         // allow write operations and read calls will always return EOF. Not 
         // very useful. It is recommended to use type == read_write instead.
         status_t open(const char* path, open_mode_t mode = open_mode_t::open_existing, open_type_t type = open_type_t::read_write) noexcept
         {
            status_t status;
            close();
            if (fd_ = CreateFileA(path, fileio::xlate_open_access(type), (FILE_SHARE_READ | FILE_SHARE_WRITE), nullptr, fileio::xlate_open_create(mode), FILE_ATTRIBUTE_NORMAL, nullptr); fd_ == INVALID_HANDLE_VALUE)
            {
               auto last_error = GetLastError();
               if (last_error == ERROR_ALREADY_EXISTS && mode == open_mode_t::create_always)
               {
                  return status;
               }
               fd_ = INVALID_HANDLE_VALUE;
               return status.reset(last_error, GetErrorMessage(last_error));
            }
            path_ = path;
            mode_ = mode;
            type_ = type;
            return status;
         }

         status_t open(const std::string& path, open_mode_t mode = open_mode_t::open_existing, open_type_t type = open_type_t::read_write) noexcept
         {
            return open(path.c_str(), mode, type);
         }

         status_t close() noexcept
         {
            status_t status;
            if (fd_ != INVALID_HANDLE_VALUE)
            {
               if (!CloseHandle(fd_))
               {
                  return status.reset(GetLastError(), GetLastErrorMessage());
               }
               fd_ = INVALID_HANDLE_VALUE;
            }
            status.clear();
            return status;
         }

         status_t write(const void* buffer, size_t size, size_t& bytes_written) const noexcept
         {
            DWORD count{};
            size_t saved_pos{ 0 };
            status_t status = save_pos(saved_pos);
            bytes_written = 0;
            if (status.nok()) return status;
            if (type_ == open_type_t::append)
            {
               if (status = seek_end(); status.nok()) return status;
            }
            if (!WriteFile(fd_, buffer, static_cast<DWORD>(size), &count, nullptr))
            {
               restore_pos(saved_pos);
               return status.reset(GetLastError(), GetLastErrorMessage());
            }
            bytes_written = count;
            return restore_pos(saved_pos);
         }

         status_t write(const char* text, size_t& bytes_written) noexcept
         {
            if (text)
            {
               return write(text, strlen(text), bytes_written);
            }
            return status_t{};
         }

         template <DataSizeContainer T>
         status_t write(const T& buffer, size_t& bytes_written) noexcept
         {
            return write(buffer.data(), buffer.size(), bytes_written);
         }

         // EOF is detected by status.ok() and bytes_read == 0
         status_t read(void* buffer, size_t size, size_t& bytes_read) noexcept
         {
            status_t status;
            DWORD count{};
            bytes_read = 0;
            if (!ReadFile(fd_, buffer, static_cast<DWORD>(size), &count, nullptr))
            {
               return status.reset(GetLastError(), GetLastErrorMessage());
            }
            bytes_read = count;
            return status;
         }

         // EOF is detected by status.ok() and bytes_read == 0
         template <DataSizeResizeContainer T>
         status_t read(T& buffer, size_t size, size_t& bytes_read) noexcept
         {
            status_t status;
            buffer.resize(size);
            bytes_read = 0;
            status = read(buffer.data(), buffer.size(), bytes_read);
            buffer.resize(bytes_read);
            return status;
         }

         [[nodiscard]]
         size_t size() const noexcept
         {
            LARGE_INTEGER lint{};
            if (fd_ != INVALID_HANDLE_VALUE && GetFileSizeEx(fd_, &lint))
            {
               return static_cast<size_t>(lint.QuadPart);
            }
            return 0;
         }

         status_t flush() const noexcept
         {
            status_t status;
            if (fd_ != INVALID_HANDLE_VALUE && !FlushFileBuffers(fd_))
            {
               return status.reset(GetLastError(), GetLastErrorMessage());
            }
            return status;
         }

         status_t seek(offset_t offset, seek_mode_t mode = seek_mode_t::begin) const noexcept
         {
            status_t status;
            LARGE_INTEGER lint{ .QuadPart = static_cast<LONGLONG>(offset) };
            if (fd_ != INVALID_HANDLE_VALUE && !SetFilePointerEx(fd_, lint, nullptr, fileio::xlate_seek_mode(mode)))
            {
               return status.reset(GetLastError(), GetLastErrorMessage());
            }
            return status;
         }

         status_t rewind() const noexcept
         {
            return seek(0, seek_mode_t::begin);
         }

         offset_t tell() const noexcept
         {
            LARGE_INTEGER current{ .QuadPart = 0 };
            LARGE_INTEGER offset{ .QuadPart = 0 };
            if (fd_ != INVALID_HANDLE_VALUE && SetFilePointerEx(fd_, current, &offset, FILE_CURRENT))
            {
               return offset.QuadPart;
            }
            return 0;

         }

         status_t lock(lock_type_t type, offset_t offset, size_t bytes, bool try_lock = false) const noexcept
         {
            status_t status;
            OVERLAPPED overlap{};
            overlap.Offset = low32(offset);
            overlap.OffsetHigh = high32(offset);
            overlap.hEvent = nullptr;
            DWORD flag{};
            if (type == lock_type_t::exclusive)
            {
               flag |= LOCKFILE_EXCLUSIVE_LOCK;
            }
            if (try_lock)
            {
               flag |= LOCKFILE_FAIL_IMMEDIATELY;
            }
            if (type == lock_type_t::unlock)
            {
               if (!UnlockFileEx(fd_, 0, low32(bytes), high32(bytes), &overlap))
               {
                  return status.reset(GetLastError(), GetLastErrorMessage());
               }
            }
            else if (!LockFileEx(fd_, flag, 0, low32(bytes), high32(bytes), &overlap))
            {
               return status.reset(GetLastError(), GetLastErrorMessage());
            }
            return status;
         }

         [[nodiscard]]
         bool try_lock(lock_type_t type, offset_t offset, size_t bytes) const noexcept
         {
            return lock(type, offset, bytes, true).ok();
         }

         status_t unlock(offset_t offset, size_t bytes) const noexcept
         {
            return lock(lock_type_t::unlock, offset, bytes, false);
         }

      private:
         [[nodiscard]]
         bool is_append() const noexcept
         {
            return (type_ == open_type_t::append);
         }

         [[nodiscard]]
         status_t seek_end() const noexcept
         {
            return seek(0, seek_mode_t::end);
         }

         status_t save_pos(size_t& saved_pos) const noexcept
         {
            if (is_append())
            {
               saved_pos = tell();
               return seek(0, seek_mode_t::end);
            }
            return status_t{};
         }

         status_t restore_pos(size_t saved_pos) const noexcept
         {
            if (is_append())
            {
               return seek(saved_pos, seek_mode_t::begin);
            }
            return status_t{};
         }

      }; // class file_t

      inline std::string GetErrorMessage(errno_t err) noexcept
      {
         LPVOID lpMsgBuf;
         FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);
         std::string msg(static_cast<char*>(lpMsgBuf));
         LocalFree(lpMsgBuf);
         return msg;
      }

      inline std::string GetLastErrorMessage() noexcept
      {
         return GetErrorMessage(GetLastError());
      }

      namespace fileio {

         inline DWORD xlate_open_access(open_type_t type) noexcept
         {
            using enum open_type_t;
            switch (type)
            {
            case read: return GENERIC_READ;
            case write: return GENERIC_WRITE;
            case read_write: return (GENERIC_READ | GENERIC_WRITE);
            case append: return (GENERIC_READ | GENERIC_WRITE);
            }
            return 0;
         }

         DWORD xlate_open_create(open_mode_t mode) noexcept
         {
            using enum open_mode_t;
            switch (mode)
            {
            case open_existing: return OPEN_EXISTING;
            case create_new: return CREATE_NEW;
            case create_always:return CREATE_ALWAYS;
            }
            return 0;
         }

         inline DWORD xlate_seek_mode(seek_mode_t mode) noexcept
         {
            using enum seek_mode_t;
            switch (mode)
            {
            case begin: return FILE_BEGIN;
            case current: return FILE_CURRENT;
            case end: return FILE_END;
            }
            return FILE_BEGIN;
         }
      } // namespace fileio

#if defined(XPLAT_OS_WIN)

   namespace fileio {

      inline status_t file_delete(const std::string& path) noexcept
      {
         status_t status;
         if (!DeleteFileA(path.c_str()))
         {
            return status.reset(GetLastError(), GetLastErrorMessage());
         }
         return status;
      }

      inline bool file_exists(const std::string& path) noexcept
      {
         DWORD fileAttributes = GetFileAttributesA(path.c_str());
         return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
      }
   } // namespace fileio

#else

      inline int GetLastError() noexcept
      {
         return errno;
      }

      inline std::string GetLastErrorMessage(errno_t err) noexcept
      {
         std::array<char, MAX_ERROR_MESSAGE_SIZE> buffer{};
         strerror_r(err, buffer.data(), buffer.size());
         return buffer.data();
      }

      inline std::string GetLastErrorMessage() noexcept
      {
         return GetLastErrorMessage(errno);
      }

   namespace fileio {

      inline status_t file_delete(const std::string& path) noexcept
      {
         status_t status;
         if (unlink(path.c_str()) != 0)
         {
            return status.reset(GetLastError(), GetLastErrorMessage());
         }
         return status;
      }

      inline bool file_exists(const std::string& path) noexcept
      {
         struct stat buffer {};
         return (stat(path.c_str(), &buffer) == 0) && S_ISREG(buffer.st_mode);
      }
   } // namespace fileio

#endif
   
} // namespace rmlib
