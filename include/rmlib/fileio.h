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

#if defined(XPLAT_OS_WIN)
   #include <windows.h>
   constexpr HANDLE NULL_HANDLE = nullptr;
   using offset_t = uint64_t;
   using errno_t = DWORD;
   
#elif defined(XPLAT_OS_LINUX)
   #define _FILE_OFFSET_BITS 64
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <unistd.h>
   using offset_t = off_t;
   using errno_t = int;
   constexpr HANDLE INVALID_HANDLE_VALUE = -1;
#else
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <unistd.h>
   using offset_t = off_t;
   using errno_t = int;
   constexpr HANDLE INVALID_HANDLE_VALUE = -1;
#endif

namespace rmlib {

   constexpr size_t MAX_ERROR_MESSAGE_SIZE = 256;
   enum class open_mode_t { open_existing = 0, create_new, create_always };
   enum class open_type_t { read = 0, write, read_write, append };
   enum class seek_mode_t { begin, current, end };
   enum class lock_type_t { shared, exclusive, unlock };             

   namespace fileio {
      
      inline errno_t last_error() noexcept;
      inline std::string last_error_message(int err) noexcept;
      inline std::string last_error_message() noexcept;
      inline HANDLE open(const char *path, open_mode_t mode, open_type_t type) noexcept;
      inline bool close(HANDLE fd) noexcept;
      inline bool write(HANDLE fd, const void* buffer, size_t size, size_t& bytes_written) noexcept;
      inline bool read(HANDLE fd, void* buffer, size_t size, size_t& bytes_read) noexcept;
      inline size_t size(HANDLE fd) noexcept;
      inline bool flush(HANDLE fd) noexcept;
      inline bool seek(HANDLE fd, offset_t offset, seek_mode_t mode) noexcept;
      inline offset_t tell(HANDLE fd) noexcept;
      inline bool lock(HANDLE fd, lock_type_t type, offset_t offset, size_t bytes, bool try_lock) noexcept;
      
   } //namespace fileio
   
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
         if (fd_ = fileio::open(path, mode, type); fd_ == INVALID_HANDLE_VALUE)
         {
            fd_ = INVALID_HANDLE_VALUE;
            return status.reset(fileio::last_error(), fileio::last_error_message());
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
         if (is_open())
         {
            status = flush();
            if (!fileio::close(fd_))
            {
               return status.reset(fileio::last_error(), fileio::last_error_message());
            }
            fd_ = INVALID_HANDLE_VALUE;
         }
         status.clear();
         return status;
      }
      
      status_t write(const void* buffer, size_t size, size_t& bytes_written) const noexcept
      {
         bytes_written = 0;
         size_t saved_pos{ 0 };
         status_t status = save_pos(saved_pos);
         if (status.nok()) return status;
         if (type_ == open_type_t::append)
         {
            if (status = seek_end(); status.nok()) return status;
         }
         if (!fileio::write(fd_, buffer, size, bytes_written))
         {
            restore_pos(saved_pos);
            return status.reset(fileio::last_error(), fileio::last_error_message());
         }
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
         bytes_read = 0;
         if (!fileio::read(fd_, buffer, size, bytes_read))
         {
            return status.reset(fileio::last_error(), fileio::last_error_message());
         }
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
         return fileio::size(fd_);
      }
      [[nodiscard]]
      status_t flush() const noexcept
      {
         status_t status;
         if (!fileio::flush(fd_))
         {
            return status.reset(fileio::last_error(), fileio::last_error_message());
         }
         return status;
      }
      
      [[nodiscard]]
      status_t seek(offset_t offset, seek_mode_t mode = seek_mode_t::begin) const noexcept
      {
         status_t status;
         if (!fileio::seek(fd_, offset, mode))
         {
            return status.reset(fileio::last_error(), fileio::last_error_message());
         }
         return status;
      }
      
      [[nodiscard]]
      status_t rewind() const noexcept
      {
         return seek(0, seek_mode_t::begin);
      }  

      [[nodiscard]]
      offset_t tell() const noexcept
      {
         return fileio::tell(fd_);
      }
      
      [[nodiscard]]
      status_t lock(lock_type_t type, offset_t offset, size_t bytes, bool try_lock = false) const noexcept
      {
         status_t status;
         if (!fileio::lock(fd_, type, offset, bytes, try_lock))
         {
            return status.reset(fileio::last_error(), fileio::last_error_message());
         }
         return status;
      }
      
      [[nodiscard]]
      bool try_lock(lock_type_t type, offset_t offset, size_t bytes) const noexcept
      {
         return lock(type, offset, bytes, true).ok();
      }
      
      [[nodiscard]]
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

   namespace fileio {

#if defined(XPLAT_OS_WIN)
      
      inline errno_t last_error() noexcept
      {
         return GetLastError();
      }
      
      inline std::string last_error_message(int err) noexcept
      {
         LPVOID lpMsgBuf;
         FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, nullptr);
         std::string msg(static_cast<char*>(lpMsgBuf));
         LocalFree(lpMsgBuf);
         return msg;
      }
      
      inline std::string last_error_message() noexcept
      {
         return GetLastErrorMessage(GetLastError());
      }

      inline status_t remove(const std::string& path) noexcept
      {
         status_t status;
         if (!DeleteFileA(path.c_str()))
         {
            return status.reset(GetLastError(), GetLastErrorMessage());
         }
         return status;
      }

      inline bool exists(const std::string& path) noexcept
      {
         DWORD fileAttributes = GetFileAttributesA(path.c_str());
         return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
      }

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
      
      inline HANDLE open(const char* path, open_mode_t mode, open_type_t type) noexcept
      {
         if (HANDLE fd = CreateFileA(path, xlate_open_access(type), (FILE_SHARE_READ | FILE_SHARE_WRITE), nullptr, xlate_open_create(mode), FILE_ATTRIBUTE_NORMAL, NULL_HANDLE); fd == INVALID_HANDLE_VALUE)
         {
            auto last_error = GetLastError();
            if (last_error == ERROR_ALREADY_EXISTS && mode == open_mode_t::create_always)
            {
               return fd;
            }
            return INVALID_HANDLE_VALUE;
         }
         return fd;
      }
      
      inline bool close(HANDLE fd) noexcept
      {
         if (fd == INVALID_HANDLE_VALUE) return true;
         return CloseHandle(fd);
      }
      
      inline bool write(HANDLE fd, const void* buffer, size_t size, size_t& bytes_written) noexcept
      {
         bytes_written = 0;
         DWORD count
         if (WriteFile(fd, buffer, static_cast<DWORD>(size), &count, nullptr))
         {
            bytes_written = count;
            return true;
         }
         return false;
      }
      
      inline bool read(HANDLE fd, void* buffer, size_t size, size_t& bytes_read) noexcept
      {
         bytes_read = 0;
         DWORD count;
         if (ReadFile(fd, buffer, static_cast<DWORD>(size), &count, nullptr))
         {
            bytes_read = count;
            return true;
         }
         return false;
      }
      
      inline size_t size(HANDLE fd) noexcept
      {
         LARGE_INTEGER lint{};
         if (GetFileSizeEx(fd, &lint))
         {
            return static_cast<size_t>(lint.QuadPart);
         }
         return 0;
      }

      inline bool flush(HANDLE fd) noexcept;
      {
         return FlushFileBuffers(fd) != 0;
      }
      
            DWORD xlate_seek_mode(seek_mode_t mode) noexcept
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

      inline bool seek(HANDLE fd, offset_t offset, seek_mode_t mode) noexcept
      {
         LARGE_INTEGER lint{ .QuadPart = static_cast<LONGLONG>(offset) };
         return SetFilePointerEx(fd, lint, nullptr, xlate_seek_mode(mode)) != 0;
      }
      
      inline offset_t tell(HANDLE fd) noexcept
      {
         LARGE_INTEGER current{ .QuadPart = 0 };
         LARGE_INTEGER offset{ .QuadPart = 0 };
         if (SetFilePointerEx(fd, current, &offset, FILE_CURRENT))
         {
            return offset.QuadPart;
         }
         return 0;
      }
      
      inline bool lock(HANDLE fd, lock_type_t type, offset_t offset, size_t bytes, bool try_lock) noexcept
      {
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
            if (!UnlockFileEx(fd, 0, low32(bytes), high32(bytes), &overlap))
            {
               return false;
            }
         }
         else
         {
            if (!LockFileEx(fd, flag, 0, low32(bytes), high32(bytes), &overlap))
            {
               return false;
            }
         }
         return true;
      }

#else
      inline errno_t last_error() noexcept
      {
         return errno;
      }
      
      inline std::string last_error_message(int err) noexcept
      {
         std::array<char, MAX_ERROR_MESSAGE_SIZE> buffer{};
         strerror_r(err, buffer.data(), buffer.size());
         return buffer.data();
      }
      
      inline std::string last_error_message() noexcept
      {
         return last_error_message(last_error());
      }
      
      inline status_t remove(const char* path) noexcept
      {
         status_t status;
         if (unlink(path) != 0)
         {
            return status.reset(last_error(), last_error_message());
         }
         return status;
      }
      
      inline bool exists(const char* path) noexcept
      {
         struct stat st{};
         return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
      }
      
      inline int xlate_open_access(open_type_t type) noexcept
      {
         using enum open_type_t;
         switch (type)
         {
            case read: return O_RDONLY;
            case write: return O_WRONLY;
            case read_write: return O_RDWR;
            case append: return O_APPEND | O_RDWR;
         }
         return 0;
      }
      
      int xlate_open_create(open_mode_t mode) noexcept
      {
         using enum open_mode_t;
         switch (mode)
         {
            case open_existing: return 0;
            case create_new: return O_CREAT | O_EXCL;
            case create_always: return O_CREAT | O_TRUNC;
         }
         return 0;
      }
      
      inline HANDLE open(const char* path, open_mode_t mode, open_type_t type) noexcept
      {
         return ::open(path, (xlate_open_create(mode) | xlate_open_access(type)), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      }

      inline bool close(HANDLE fd) noexcept
      {
         if (fd == INVALID_HANDLE_VALUE) return true;
         return ::close(fd) == 0;
      }
      
      inline bool write(HANDLE fd, const void* buffer, size_t size, size_t& bytes_written) noexcept
      {
         bytes_written = 0;
         ssize_t count = ::write(fd, buffer, size);
         if (count >= 0)
         {
            bytes_written = count;
         }
         return (count >= 0);
      }
      
      inline bool read(HANDLE fd, void* buffer, size_t size, size_t& bytes_read) noexcept
      {
         bytes_read = 0;
         ssize_t count = ::read(fd, buffer, size);
         if (count >= 0)
         {
            bytes_read = count;
         }
         return (count >= 0);
      }
      
      inline size_t size(HANDLE fd) noexcept
      {
         struct stat st{};
         if (fstat(fd, &st) == 0)
         {
            return st.st_size;
         }
         return 0;
      }
      
      inline bool flush(HANDLE fd) noexcept
      {
         return fsync(fd) == 0;
      }
      
      int xlate_seek_mode(seek_mode_t mode) noexcept
      {
         using enum seek_mode_t;
         switch (mode)
         {
            case begin: return SEEK_SET;
            case current: return SEEK_CUR;
            case end: return SEEK_END;
         }
         return SEEK_SET;
      }
      
      inline bool seek(HANDLE fd, offset_t offset, seek_mode_t mode) noexcept
      {
         return lseek(fd, offset, fileio::xlate_seek_mode(mode)) != -1;
      }
      
      inline offset_t tell(HANDLE fd) noexcept
      {
         if (off_t ret = lseek(fd, 0, SEEK_CUR); ret != -1)
         {
            return ret;
         }
         return 0;
      }
      
      inline bool lock(HANDLE fd, lock_type_t type, offset_t offset, size_t bytes, bool try_lock) noexcept
      {
         struct flock fl{};
         fl.l_type = (type == lock_type_t::shared) ? F_RDLCK : F_WRLCK;
         fl.l_whence = SEEK_SET;
         fl.l_start = offset;
         fl.l_len = bytes;
         fl.l_pid = getpid();
         int cmd = try_lock ? F_OFD_SETLK : F_OFD_SETLKW;
         return (fcntl(fd, cmd, &fl) != -1);
      }

#endif

   } // namespace file

} // namespace rmlib
