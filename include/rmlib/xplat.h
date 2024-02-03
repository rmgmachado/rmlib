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

