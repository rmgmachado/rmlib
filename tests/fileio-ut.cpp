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
#include <thread>
#include <atomic>
#include <functional>

#include "rmlib/fileio.h"

using namespace rmlib;

namespace fileio_ut {

   constexpr const char* path1 = "test1.txt";
   constexpr const char* path2 = "test2.txt";

   constexpr const char* text1 = "This is a test string1\n";
   constexpr const char* text2 = "This is a test string2 a bit longer\n";

   void cleanup() noexcept
   {
      fileio::remove("test1.txt");
      fileio::remove("test2.txt");
   }

   status_t create_file(const char* path, const char* text, size_t& bytes_written) noexcept
   {
      file_t file;
      bytes_written = 0;
      status_t status = file.open(path, open_mode_t::create_new, open_type_t::read_write);
      if (status.ok())
      {
         if (status = file.write(text, bytes_written); status.ok())
         {
            status = file.close();
         }
      }
      return status;
   }

   status_t create_file(const char* path, const char* text) noexcept
   {
      size_t bytes_written{};
      return create_file(path, text, bytes_written);
   }

} // namespace fileio_ut

TEST_CASE("rmlib::fileio unit tests", "[fileio]")
{
   fileio_ut::cleanup();
   SECTION("Test rmlib::file_t default constructor")
   {
      file_t file;
      REQUIRE(!file.is_open());
      REQUIRE(file.path() == "");
      REQUIRE(file.handle() == INVALID_HANDLE_VALUE);
      REQUIRE(file.size() == 0);
   }
   SECTION("Test file_t.open(open_existing, read) with unexistent file")
   {
      file_t file;
      // open should file to open an unexistent file
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::read).nok());
      REQUIRE(!file.is_open());
      REQUIRE(file.path() == "");
      REQUIRE(file.handle() == INVALID_HANDLE_VALUE);
      REQUIRE(file.size() == 0);
      // create the file first and write some text
      size_t bytes_written{};
      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      // now open the file
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::read).ok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, bytes_written, bytes_read).ok());
      REQUIRE(bytes_read == bytes_written);
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test file_t.open(open_existing, read) for read and try to write to it")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::read).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).nok());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test file_t.open(open, write) and write and read")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::write).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, strlen(fileio_ut::text2), bytes_read).nok());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test file_t.open(open_existing, read_write) and write and read")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::read_write).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      // reading past the end of file
      REQUIRE(file.read(text, strlen(fileio_ut::text2), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(text.empty());
      // rewind and try again
      REQUIRE(file.rewind().ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text2), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text2));
      REQUIRE(text == fileio_ut::text2);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test file_t.open(open_existing, append) and write and read")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::append).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      // reading pointer at the beginning of the file
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text1));
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.read(text, strlen(fileio_ut::text2), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text2));
      REQUIRE(text == fileio_ut::text2);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_new, read) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_new, open_type_t::read).ok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).nok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_new, write) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_new, open_type_t::write).ok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).nok());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_new, read_write) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_new, open_type_t::read_write).ok());
      file_t file1;
      // try to create the file again. should fail
      REQUIRE(file1.open(fileio_ut::path1, open_mode_t::create_new, open_type_t::read_write).nok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(text.empty());
      REQUIRE(file.rewind().ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text1));
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_new, append) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_new, open_type_t::append).ok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).ok());
      size_t bytes_read{};
      std::string text;
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text1));
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::exists(fileio_ut::path1));
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_always, read) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      size_t bytes_read{};
      std::string text;

      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_always, open_type_t::read).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).nok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(text.empty());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_always, write) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      size_t bytes_read{};
      std::string text;

      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_always, open_type_t::write).ok());
      REQUIRE(file.write(fileio_ut::text2, bytes_written).ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text2), bytes_read).nok());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_always, read_write) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      size_t bytes_read{};
      std::string text;

      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_always, open_type_t::read_write).ok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(text.empty());
      REQUIRE(file.rewind().ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text1));
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_always, append) and read and write")
   {
      file_t file;
      size_t bytes_written{};
      size_t bytes_read{};
      std::string text;

      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_always, open_type_t::append).ok());
      REQUIRE(file.write(fileio_ut::text1, bytes_written).ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == strlen(fileio_ut::text1));
      REQUIRE(text == fileio_ut::text1);
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::open(create_always, read_write) when file exists and has data")
   {
      file_t file;
      size_t bytes_read{};
      std::string text;

      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::create_always, open_type_t::read_write).ok());
      REQUIRE(file.read(text, strlen(fileio_ut::text1), bytes_read).ok());
      REQUIRE(bytes_read == 0);
      REQUIRE(text.empty());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());
   }
   SECTION("Test fileio::seek() and fileio::tell()")
   {
      file_t file;
      size_t bytes_read{};
      std::string text;
      offset_t offset{ 15 };
      std::string str{"string1\n"};

      REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1).ok());
      REQUIRE(file.open(fileio_ut::path1, open_mode_t::open_existing, open_type_t::read_write).ok());
      REQUIRE(file.seek(offset, seek_mode_t::begin).ok());
      REQUIRE(file.read(text, 100, bytes_read).ok());
      REQUIRE(bytes_read == str.size());
      REQUIRE(text == str);
      REQUIRE(file.tell() == offset + str.size());
      REQUIRE(file.seek(0, seek_mode_t::current).ok());
      REQUIRE(file.tell() == offset + str.size());
      REQUIRE(file.close().ok());
      REQUIRE(fileio::remove(fileio_ut::path1).ok());

   }
}

namespace fileio_ut {

   void sleep_ms(int ms) noexcept
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
   }

   void lock_thread_function(std::stop_token stoken, file_t& file, lock_type_t lock_type) noexcept
   {
      status_t status = file.lock(lock_type, 0, 1);
      if (status.nok()) return;
      while (!stoken.stop_requested())
      {
         sleep_ms(1);
      }
      status = file.unlock(0, 1);
   }

   class thread_t {
      std::jthread thread_;
      std::atomic_bool running_{ false };

   public:
      thread_t() = default;

      // Constructor that takes a function to execute
      thread_t(std::function<void(std::stop_token)> tfunc) 
         : thread_([this, tfunc](std::stop_token stopToken) 
         {
            running_ = true;
            tfunc(stopToken);
            running_ = false;
         }) 
      {}

      // Stop the thread
      void stop() noexcept 
      {
         thread_.request_stop();
      }

      // Check if the thread is running
      bool is_running() const noexcept 
      {
         return running_.load();
      }

      // Check if the thread is joinable
      bool is_joinable() const noexcept 
      {
         return thread_.joinable();
      }

      // Join the thread
      void join() noexcept 
      {
         thread_.join();
      }

      // Get the ID of the thread
      std::jthread::id get_id() const noexcept 
      {
         return thread_.get_id();
      }
   };


} // namespace fileio_ut

#ifdef RICARDO_REMOVED_FOR_NOW
TEST_CASE("Test rmlib::file locking", "[fileio]")
{
   fileio_ut::cleanup();
   file_t file;
   size_t bytes_written{};
   lock_type_t lock_type{ lock_type_t::exclusive };
   REQUIRE(fileio_ut::create_file(fileio_ut::path1, fileio_ut::text1, bytes_written).ok());
   REQUIRE(file.open(fileio_ut::path1).ok());
   fileio_ut::thread_t thread([&](std::stop_token token)
   {
      fileio_ut::lock_thread_function(token, file, lock_type);
   });
   // wait for thread to start
   while (!thread.is_running())
   {
      fileio_ut::sleep_ms(1);
   }
   fileio_ut::sleep_ms(50);
   REQUIRE(!file.try_lock(lock_type, 0, 1));
   // signal thread to stop
   thread.stop();
   // wait for thread to stop
   while (thread.is_running())
   {
      fileio_ut::sleep_ms(1);
   }
   fileio_ut::sleep_ms(50);
   REQUIRE(file.try_lock(lock_type, 0, 1));
   REQUIRE(file.unlock(0, 1).ok());
   REQUIRE(file.close().ok());
   REQUIRE(fileio::remove(fileio_ut::path1).ok());
}
#endif
