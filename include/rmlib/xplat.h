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

/*****************************************************************************\
*
*  Supported Operating Systems
*
\*****************************************************************************/
#if defined(_WIN32) || defined(_WIN64)
   #define XPLAT_OS_WIN
   #define XPLAT_OS_WINDOWS
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "Windows"
   #if defined(_WIN32)
      #define XPLAT_OS_WIN32
   #endif
   #if defined(_WIN64)
      #define XPLAT_OS_WIN64
   #endif
#endif

#if defined(__linux__) 
   #define XPLAT_OS_LINUX
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "Linux"
#endif

#if defined(__APPLE__) && defined(__MACH__)
   #define XPLAT_OS_MACOS
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "MacOS X"
#endif

#if defined(__FreeBSD__)
   #define XPLAT_OS_FREEBSD
   #define XPLAT_OS_BSD
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "FreeBSD"
#endif

#if defined(__NetBSD__)
   #define XPLAT_OS_NETBSD
   #define XPLAT_OS_BSD
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "NetBSD"
#endif

#if defined(__OpenBSD__)
   #define XPLAT_OS_FREEBSD
   #define XPLAT_OS_BSD
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "OpenBSD"
#endif

#if defined(__bsdi__) || defined(__DragonFly__)
   #define XPLAT_OS_BSD
   #define XPLAT_OS_UNIX
   #define XPLAT_OS_SUPPORTED
   #define XPLAT_OS_NAME   "DragonFly"
#endif

/*****************************************************************************\
*
* Supported C/C++ Compilers
*
\*****************************************************************************/
#ifdef _MSC_VER
   #define XPLAT_CC_MSVC      _MSC_VER   
   #define XPLAT_CC_SUPPORTED 
   #define XPLAT_CC_NAME      "Visual Studio"
#endif

#if defined(__GCC__)
   #define XPLAT_CC_GCC (__GNUC__ * 100 + __GNUC_MINOR__)
   #define XPLAT_CC_SUPPORTED 
   #define XPLAT_CC_NAME      "GNU GCC"
#endif

#if defined(__clang__)
   #define XPLAT_CC_CLANG
   #define XPLAT_CC_GCC
   #define XPLAT_CC_SUPPORTED 
   #define XPLAT_CC_NAME      "CLang"
#endif

/*****************************************************************************\
*
*  Support CPU platforms
*
\*****************************************************************************/
#if defined(__amd64__) || defined(__amd64) || defined(__X86_64__) || defined(__x86_64) || defined(_M_AMD64)
   #define XPLAT_CPU_AMD64
   #define XPLAT_CPU_IX64
   #define XPLAT_CPU_SUPPORTED
   #define XPLAT_CPU_NAME  "AMD x64"
#endif

#if defined(__arm__) || defined(_M_ARM) || defined(_M_ARMT) || defined(__aarch64__)
   #define XPLAT_CPU_ARM
   #define XPLAT_CPU_SUPPORTED
   #define XPLAT_CPU_NAME  "ARM"
   #if defined(__aarch64__)
      #define XPLAT_CPU_ARM64
   #endif
#endif

#if defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
   #if !defined(XPLAT_CPU_IX64)
      #define XPLAT_CPU_IX32
      #define XPLAT_CPU_SUPPORTED
      #define XPLAT_CPU_NAME  "Intel i32"
   #endif
#endif

#if !defined(XPLAT_OS_WIN)
   #ifndef _GNU_SOURCE
      #define _GNU_SOURCE
   #endif
   #include <string.h>
   #include <cerrno>

   inline int strerror_s(char* buf, size_t buflen, int errnum) noexcept
   {
      if (!buf || buflen == 0) return EINVAL;
      char* str = strerror_r(errnum, buf, buflen);
      if (str != buf)
      {
         strncpy(buf, str, buflen);
         buf[buflen - 1] = '\0';
      }
      return 0;
   }

   using SOCKET = int;
   using HANDLE = int;
#endif

#if defined(XPLAT_OS_WINDOWS)
   #include <Windows.h>
   #include <cerrno>
   #include <string>

   using off64_t = long long;

   inline std::string get_windows_error_message(DWORD dwError) noexcept
   {
      LPVOID lpMsgBuf;
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
      std::string strMsg((LPSTR)lpMsgBuf);
      LocalFree(lpMsgBuf);
      return strMsg;
   }

   inline std::string get_windows_last_error_message() noexcept
   {
      return get_windows_error_message(GetLastError());
   }

   inline int xlate_windows_error_code(DWORD dwError) noexcept
   {
      switch (dwError)
      {
         case ERROR_FILE_NOT_FOUND:             return ENOENT;
         case ERROR_PATH_NOT_FOUND:             return ENOENT;
         case ERROR_TOO_MANY_OPEN_FILES:        return EMFILE;
         case ERROR_ACCESS_DENIED:              return ENODATA;
         case ERROR_INVALID_HANDLE:             return EBADF;
         case ERROR_NOT_ENOUGH_MEMORY:          return ENOMEM;
         case ERROR_INVALID_ACCESS:             return ENODATA;
         case ERROR_INVALID_DATA:               return EINVAL;
         case ERROR_OUTOFMEMORY:                return ENOMEM;
         case ERROR_WRITE_PROTECT:              return EACCES;
         case ERROR_BAD_LENGTH:                 return EINVAL;
         case ERROR_SEEK:                       return ESPIPE;
         case ERROR_SHARING_VIOLATION:          return EACCES;
         case ERROR_LOCK_VIOLATION:             return ENOLCK;
         case ERROR_SHARING_BUFFER_EXCEEDED:    return ENOBUFS;
         case ERROR_NOT_SUPPORTED:              return ENOTSUP;
         case ERROR_DEV_NOT_EXIST:              return ENODATA;
         case ERROR_BAD_DEV_TYPE:               return ENODEV;
         case ERROR_FILE_EXISTS:                return EEXIST;
         case ERROR_BROKEN_PIPE:                return EPIPE;
         case ERROR_BUFFER_OVERFLOW:            return ENOBUFS;
         case ERROR_INVALID_TARGET_HANDLE:      return EBADF;
         case ERROR_CALL_NOT_IMPLEMENTED:       return ENOSYS;
         case ERROR_SEM_TIMEOUT:                return ETIME;
         case ERROR_INSUFFICIENT_BUFFER:        return ENOBUFS;
         case ERROR_INVALID_NAME:               return ENODATA;
         case ERROR_PROC_NOT_FOUND:             return ENOSYS;
         case ERROR_NEGATIVE_SEEK:              return EPIPE;
         case ERROR_SEEK_ON_DEVICE:             return EPIPE;
         case ERROR_DIR_NOT_EMPTY:              return ENOTEMPTY;
         case ERROR_BAD_ARGUMENTS:              return EINVAL;
         case ERROR_BAD_PATHNAME:               return ENOENT;
         case ERROR_BUSY:                       return EBUSY;
         case ERROR_ALREADY_EXISTS:             return EALREADY;
         case ERROR_FILENAME_EXCED_RANGE:       return ENAMETOOLONG;
         case ERROR_BAD_FILE_TYPE:              return EBADF;
         case ERROR_FILE_TOO_LARGE:             return ENAMETOOLONG;
         case ERROR_BAD_PIPE:                   return EPIPE;
         case ERROR_PIPE_BUSY:                  return EBUSY;
         case ERROR_NO_DATA:                    return ENODATA;
         case WAIT_TIMEOUT:                     return ETIMEDOUT;
         case ERROR_DIRECTORY:                  return ENOENT;
         case ERROR_NOT_OWNER:                  return EPERM;
         default: return EINVAL;
      }
      return EINVAL;
   }
#endif

