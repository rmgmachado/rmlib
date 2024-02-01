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
#include <catch.hpp>

#include "rmlib/xplat.h"
#include "rmlib/status.h"

#if defined(XPLAT_OS_WIN)
   #include <fcntl.h>
   #include <io.h>
   #define OPEN_FILE _open
   #define OPEN_MODE _O_RDWR
#else
   #include <fcntl.h>
   #define OPEN_FILE open
   #define OPEN_MODE O_RDWR
#endif

using namespace rmlib;

namespace status_ut {

   #if defined(XPLAT_CC_MSVC)
      #pragma warning(push)
      #pragma warning(disable : 4996)
   #endif
   int open_file(const char* path) noexcept
   {
      return OPEN_FILE(path, OPEN_MODE);
   }
   #if defined(XPLAT_CC_MSVC)
      #pragma warning(pop)
   #endif

} // namespace status_ut

TEST_CASE("rmstatus unit tests", "[rmstatus]")
{
   SECTION("test rmlib::status_t default constructor")
   {
      status_t status;
      REQUIRE(status.ok());
      REQUIRE(!status.nok());
      REQUIRE(status.error() == 0);
      REQUIRE(status.reason() == "No errors detected");
   }
   SECTION("test rmlib::status_t constructor passing a 0")
   {
      status_t status{ 0 };
      REQUIRE(status.ok());
      REQUIRE(!status.nok());
      REQUIRE(status.error() == 0);
      REQUIRE(status.reason() == "No errors detected");
   }
   SECTION("test rmlib::status_t assigning a 0")
   {
      status_t status;
      status = 0;
      REQUIRE(status.ok());
      REQUIRE(!status.nok());
      REQUIRE(status.error() == 0);
      REQUIRE(status.reason() == "No errors detected");
   }
   SECTION("test rmlib::status_t constructor passing a ENOENT error")
   {
      status_t status{ ENOENT };
      REQUIRE(!status.ok());
      REQUIRE(status.nok());
      REQUIRE(status.error() == ENOENT);
      REQUIRE(status.reason() == "No such file or directory");
      status.clear();
      REQUIRE(status.ok());
   }
   SECTION("test rmlib::status_t constructor assigning a ENOENT error")
   {
      status_t status = ENOENT;
      REQUIRE(!status.ok());
      REQUIRE(status.nok());
      REQUIRE(status.error() == ENOENT);
      REQUIRE(status.reason() == "No such file or directory");
      status.clear();
      REQUIRE(status.ok());
   }
   SECTION("test rmlib::status_t assigning a ENOENT error")
   {
      status_t status;
      status = ENOENT;
      REQUIRE(!status.ok());
      REQUIRE(status.nok());
      REQUIRE(status.error() == ENOENT);
      REQUIRE(status.reason() == "No such file or directory");
   }
   SECTION("test rmlib::status_t constructor passing a -1 from a failed operator")
   {
      status_t status{ status_ut::open_file("non-existing-file-just-for-this-test") };
      REQUIRE(!status.ok());
      REQUIRE(status.nok());
      REQUIRE(status.error() == ENOENT);
      REQUIRE(status.reason() == "No such file or directory");
   }
   SECTION("test rmlib::status_t constructor passing a -1 from a failed operator")
   {
      status_t status;
      status = status_ut::open_file("non-existing-file-just-for-this-test");
      REQUIRE(!status.ok());
      REQUIRE(status.nok());
      REQUIRE(status.error() == ENOENT);
      REQUIRE(status.reason() == "No such file or directory");
   }
}

