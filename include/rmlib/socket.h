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

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <atomic>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>

#include "rmlib/xplat.h"
#include "rmlib/utility.h"
#include "rmlib/time.h"

/*****************************************************************************\
*
*  Windows specific definitionss
*
\*****************************************************************************/
#if defined(XPLAT_OS_WIN)
   #include <winsock2.h>
   #include <ws2tcpip.h>

   #include "wepoll.h"

   #define WSAEAGAIN WSAEWOULDBLOCK
   constexpr HANDLE INVALID_EPOLL_HANDLE = nullptr;

   // link with Ws2_32.lib
   #pragma comment (lib, "Ws2_32.lib")
#endif

/*****************************************************************************\
*
*  Linux specific definitions
*
\*****************************************************************************/
#if defined(XPLAT_OS_LINUX)
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <sys/ioctl.h>
   #include <sys/epoll.h>
   #include <arpa/inet.h>
   #include <netdb.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <poll.h>
   #include <errno.h>
   #include <string.h>

   using WSAPOLLFD = struct pollfd;
   using LPWSAPOLLFD = struct pollfd*;
   using ADDRINFOA = struct addrinfo;
   using PADDRINFOA = struct addrinfo*;
   using SOCKADDR = struct sockaddr;

   inline int WSAGetLastError() noexcept { return errno; }
   inline int closesocket(SOCKET fd) noexcept { return ::close(fd); }
   inline int ioctlsocket(SOCKET fd, long cmd, u_long* argp) noexcept { return ::ioctl(fd, cmd, argp); }
   inline int WSAPoll(LPWSAPOLLFD fdArray, nfds_t nfds, int timeout) { return ::poll(fdArray, nfds, timeout); }
   inline void ZeroMemory(void* ptr, size_t size) { memset(ptr, '\0', size); }
   inline int epoll_close(HANDLE ephnd) { return close(ephnd); }

   constexpr int INVALID_SOCKET = -1;
   constexpr int SOCKET_ERROR = -1;
   constexpr HANDLE INVALID_EPOLL_HANDLE = -1;

   #define WSAEWOULDBLOCK  EWOULDBLOCK
   #define WSAECONNREFUSED ECONNREFUSED
   #define WSAEHOSTUNREACH ENETUNREACH 
   #define WSAEAGAIN       EAGAIN
   #define WSAEINVAL       EINVAL
   #define WSAENOTSOCK     ENOTSOCK
   #define WSAEALREADY     EALREADY
   #define WSAENOTCONN     ENOTCONN

   #define SD_SEND      SHUT_WR
   #define SD_RECEIVE   SHUT_RD
   #define SD_BOTH      SHUT_RDWR
#endif


namespace rmlib {

   constexpr size_t SOCKET_MAX_ERROR_STRING_SIZE = 512;
   enum class status_code_t { none, closing, want_read, want_write, io, fatal };

   namespace socket {

      class status_t
      {
         int error_{};
         unsigned long ctx_{ 0 };
         status_code_t code_{ status_code_t::none };

      public:
         status_t() noexcept = default;

         // construct socket errors
         status_t(int error, status_code_t hint = status_code_t::io) noexcept
            : error_{ error }
            , code_{ (error == 0) ? status_code_t::none : hint }
         {
            if (error_ == SOCKET_ERROR)
            {
               code_ = status_code_t::io;
               error_ = WSAGetLastError();
            }
            if (error_ == WSAEWOULDBLOCK && hint != status_code_t::io)
            {
               code_ = hint;
            }
            if (error_ == 0 && hint == status_code_t::closing)
            {
               code_ = hint;
            }
         }

         // construct SSL errors
         status_t(const SSL* ssl, int ret) noexcept
            : error_{ SSL_get_error(ssl, ret) }
         {
            using enum status_code_t;
            switch (error_)
            {
            case SSL_ERROR_NONE: error_ = 0; break;
            case SSL_ERROR_ZERO_RETURN: code_ = closing; break;
            case SSL_ERROR_WANT_READ: code_ = want_read; break;
            case SSL_ERROR_WANT_WRITE: code_ = want_write; break;
            case SSL_ERROR_SYSCALL:
            {
               if (ERR_peek_last_error() != 0)
               {
                  status_t(fatal);
               }
               else
               {
                  status_t{ WSAGetLastError() };
               }
               break;
            }
            default: status_t(fatal); break;
            }
         }

         // construct SSL CTX errors
         explicit status_t(status_code_t) noexcept
            : ctx_{ ERR_peek_last_error() }
         {
            if (ctx_ != 0)
            {
               error_ = ERR_GET_REASON(ctx_);
               code_ = status_code_t::fatal;
            }
            ERR_clear_error();
         }

         bool ok() const noexcept
         {
            return code_ == status_code_t::none;
         }

         bool nok() const noexcept
         {
            return !ok();
         }

         status_code_t code() const noexcept
         {
            return code_;
         }

         bool would_block() const noexcept
         {
            using enum status_code_t;
            return code_ == want_read || code_ == want_write;
         }

         bool want_read() const noexcept
         {
            using enum status_code_t;
            return code_ == want_read;
         }

         bool want_write() const noexcept
         {
            using enum status_code_t;
            return code_ == want_write;
         }

         void clear() noexcept
         {
            error_ = 0;
            ctx_ = 0;
            code_ = status_code_t::none;
            ERR_clear_error();
         }

         [[nodiscard]]
         std::string reason() const noexcept
         {
            using enum status_code_t;
            if (code_ == fatal)
            {
               std::array<char, SOCKET_MAX_ERROR_STRING_SIZE> buffer{};
               ERR_error_string_n(ctx_, buffer.data(), buffer.size());
               return std::string{buffer.data() };
            }
            else if (code_ != none)
            {
               std::array<char, SOCKET_MAX_ERROR_STRING_SIZE> text{};
               strerror_s(text.data(), text.size(), error_);
               return std::string{text.data() };
            }
            return "No errors detected";
         }
      }; // class status_t

   } // namespace socket

   namespace ip {

      struct info_t
      {
         static int last_error() noexcept
         {
            return ::WSAGetLastError();
         }

         static int getnameinfo(const SOCKADDR* addr, socklen_t addr_len, char* host, unsigned host_len, char* port, unsigned port_len, int flags) noexcept
         {
            return (::getnameinfo(addr, addr_len, host, host_len, port, port_len, flags) == 0) ? 0 : last_error();
         }

         static int gethostname(char* name, int name_len) noexcept
         {
            return ::gethostname(name, name_len) == SOCKET_ERROR ? last_error() : 0;
         }

         static int getaddrinfo(const char* host, const char* port, const ADDRINFOA* hints, PADDRINFOA* results) noexcept
         {
            return ::getaddrinfo(host, port, hints, results);
         }

         static void freeaddrinfo(PADDRINFOA info) noexcept
         {
            ::freeaddrinfo(info);
         }
      }; // struct info_t

      class address_t
      {
         sockaddr addr_{};
         socklen_t len_{};

      public:
         address_t() = default;
         ~address_t() = default;
         address_t(const address_t&) = default;
         address_t(address_t&&) = default;
         address_t& operator=(const address_t&) = default;
         address_t& operator=(address_t&&) = default;

         address_t(const sockaddr* addr, socklen_t len) noexcept
            : addr_{ addr ? *addr : sockaddr{} }
            , len_{ len }
         {}

         address_t(const sockaddr& addr, socklen_t len) noexcept
            : addr_{ addr }
            , len_{ len }
         {}

         friend void swap(address_t& lhs, address_t& rhs) noexcept
         {
            std::swap(lhs.addr_, rhs.addr_);
            std::swap(lhs.len_, rhs.len_);
         }

         const sockaddr* address() const noexcept
         {
            return &addr_;
         }

         socklen_t length() const noexcept
         {
            return len_;
         }

         int family() const noexcept
         {
            return addr_.sa_family;
         }

         unsigned port() const noexcept
         {
            auto [port, notation] = notation_and_port();
            return port;
         }

         std::string dot_notation() const noexcept
         {
            auto [port, notation] = notation_and_port();
            return notation;
         }

         std::string url() const noexcept
         {
            return dot_notation() + ":" + std::to_string(port());
         }

      private:
         std::pair<uint16_t, std::string> notation_and_port() const noexcept
         {
            std::array<char, INET6_ADDRSTRLEN> ipStr{ "Unkown AF" };
            uint16_t port_num{};

            if (addr_.sa_family == AF_INET) // IPv4
            {
               auto addr_in = reinterpret_cast<const struct sockaddr_in*>(&addr_);
               inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr.data(), ipStr.size());
               port_num = ntohs(addr_in->sin_port);

            }
            else if (addr_.sa_family == AF_INET6) // IPv6
            {
               auto addr_in6 = reinterpret_cast<const struct sockaddr_in6*>(&addr_);
               inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr.data(), ipStr.size());
               port_num = ntohs(addr_in6->sin6_port);
            }
            return std::make_pair(port_num, std::string(ipStr.data()));
         }
      }; // class address_t

      /***********************************************************************\
      *
      *  Retrieve the peer name from ipaddress_t in the format "host:port".
      *  Returns an empty string if the host name cannot be retrieved
      *
      \***********************************************************************/
      template <typename T = ip::info_t>
      inline socket::status_t peer_name(const address_t& address, std::string& name) noexcept
      {
         std::array<char, NI_MAXHOST> host = {};
         std::array<char, NI_MAXSERV> port = {};
         int ret{ 0 };
         if (ret = T::getnameinfo(address.address(), address.length(), host.data(), (socklen_t)host.size(), port.data(), (socklen_t)port.size(), (NI_NUMERICHOST | NI_NUMERICSERV)); ret == 0)
         {
            name = std::string(host.data()) + std::string(":") + std::string(port.data());
            return socket::status_t();
         }
         return socket::status_t(ret);
      }

      /*****************************************************************************\
      *
      *  local_host_name()
      *  Retrieve the name of the local host. Returns an empty string if the local
      *  host name cannot be retrieved
      *
      \*****************************************************************************/
      template <typename T = ip::info_t>
      inline socket::status_t local_host_name(std::string& host) noexcept
      {
         constexpr const int HN_MAX_HOSTNAME = 256;
         std::array<char, HN_MAX_HOSTNAME> name = {};
         if (T::gethostname(&name[0], (int)name.size()) == 0)
         {
            host = std::string(name.data());
            return socket::status_t();
         }
         return socket::status_t{ T::last_error() };
      }

      inline std::string local_host_name() noexcept
      {
         std::string host;
         if (local_host_name(host).ok())
         {
            return host;
         }
         host.clear();
         return host;
      }

      using address_list_t = std::vector<address_t>;
      enum class resolution_type_t : int { normal = 0, passive = AI_PASSIVE };

      template <typename T = ip::info_t>
      inline socket::status_t address_resolution(const std::string& host, const std::string& port, address_list_t& address_list, resolution_type_t type = resolution_type_t::normal) noexcept
      {
         PADDRINFOA addr{ nullptr };
         PADDRINFOA ptr{ nullptr };
         ADDRINFOA hints;
         ZeroMemory(&hints, sizeof(hints));
         hints.ai_flags = (int)type;
         hints.ai_family = AF_UNSPEC;
         hints.ai_socktype = SOCK_STREAM;
         hints.ai_protocol = IPPROTO_TCP;
         int retval = T::getaddrinfo(host.c_str(), port.c_str(), &hints, &addr);
         if (retval == 0)
         {
            address_list.clear();
            ptr = addr;
            while (ptr != nullptr)
            {
               if (ptr->ai_addr != nullptr && ptr->ai_addrlen > 0)
               {
                  address_list.emplace_back(address_t(ptr->ai_addr, (int)ptr->ai_addrlen));
               }
               ptr = ptr->ai_next;
            }
         }
         T::freeaddrinfo(addr);
         return socket::status_t{ retval };
      }

      // resolve IP name and port in the format hostname:port
      inline socket::status_t address_resolution(const std::string& host_and_port, address_list_t& address_list, resolution_type_t type = resolution_type_t::normal) noexcept
      {
         auto separator_pos = host_and_port.find_first_of(':');
         if (separator_pos != std::string::npos)
         {
            return address_resolution(host_and_port.substr(0, separator_pos), host_and_port.substr(separator_pos + 1), address_list, type);
         }
         return socket::status_t(WSAEINVAL);
      }

   } // namespace ip

   namespace tls {

      enum class context_type_t { client, server };

      class context_t
      {
         SSL_CTX* ctx_{ nullptr };
        socket::status_t status_;

      public:
         context_t() = default;
         context_t(const context_t&) = delete;
         context_t(context_t&&) noexcept = default;
         context_t& operator=(const context_t&) = delete;
         context_t& operator=(context_t&&) noexcept = default;

         explicit context_t(context_type_t type) noexcept
         {
            create(type, nullptr, nullptr);
         }

         context_t(context_type_t type, const char* pem_file) noexcept
         {
            create(type, pem_file, pem_file);
         }

         context_t(context_type_t type, const char* cert_pem_file, const char* key_pem_file) noexcept
         {
            create(type, cert_pem_file, key_pem_file);
         }

        socket::status_t set_verify(const char* pem_file) noexcept
         {
            status_ =socket::status_t();
            SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, nullptr);
            SSL_CTX_set_verify_depth(ctx_, 4);
            if (int ret = SSL_CTX_load_verify_locations(ctx_, pem_file, nullptr); ret != 1)
            {
               status_ =socket::status_t(status_code_t::fatal);
            }
            return status_;
         }

         ~context_t() noexcept
         {
            if (ctx_)
            {
               SSL_CTX_free(ctx_);
               ctx_ = nullptr;
            }
         }

         SSL_CTX* operator()() noexcept
         {
            return ctx_;
         }

        socket::status_t status() const noexcept
         {
            return status_;
         }

      private:
         void create(context_type_t type, const char* cert_pem_file, const char* key_pem_file) noexcept
         {
            status_.clear();
            allocate_context(type);
            set_certificate(cert_pem_file);
            set_private_key(key_pem_file);
            if (status_.nok())
            {
               free_context();
            }
         }

         void allocate_context(context_type_t type) noexcept
         {
            if (ctx_ = SSL_CTX_new(type == context_type_t::client ? TLS_client_method() : TLS_server_method()); !ctx_)
            {
               status_ =socket::status_t(status_code_t::fatal);
            }
         }

         void free_context() noexcept
         {
            if (ctx_)
            {
               SSL_CTX_free(ctx_);
               ctx_ = nullptr;
            }
         }

         void set_certificate(const char* cert_pem_file) noexcept
         {
            if (status_.ok() && cert_pem_file && SSL_CTX_use_certificate_file(ctx_, cert_pem_file, SSL_FILETYPE_PEM) <= 0)
            {
               status_ =socket::status_t(status_code_t::fatal);
            }
         }

         void set_private_key(const char* key_pem_file) noexcept
         {
            if (status_.ok() && key_pem_file && SSL_CTX_use_PrivateKey_file(ctx_, key_pem_file, SSL_FILETYPE_PEM) <= 0)
            {
               status_ =socket::status_t(status_code_t::fatal);
            }
         }

      }; // class context_t

   } // namespace tls

   constexpr size_t KBytes(size_t n) noexcept { return n * 1024; }
   constexpr size_t MBytes(size_t n) noexcept { return n * 1024 * 1024; }

   using uid_t = uint64_t;

   enum class socket_event_t { recv_ready, send_ready, connect_ready, accept_ready };
   enum class socket_mode_t { blocking, nonblocking };
   enum class socket_close_t : int { send = SD_SEND, recv = SD_RECEIVE, both = SD_BOTH };

   enum class socket_state_t { 
        idle         // socket_t object not connected or closed
      , created      // underlying SOCKET handle created
      , connecting   // TLS handshake in progress for connect
      , connected    // socket_t connected
      , listening    // spocket_t is listening for connections
      , accepting    // TLS handshake in progress for accept
   };
   
   using wait_timeout_t = int;
   constexpr wait_timeout_t SOCKET_WAIT_FOREVER = -1L;
   constexpr wait_timeout_t SOCKET_WAIT_NEVER = 0L;

   // socket_recv_buffer_size controls the size of a buffer in the stack
   // when socket_base_t::recv(std::string&) is called. Small values will 
   // require more calls to ::recv to receive all data in sockets buffer
   constexpr size_t SOCKET_DEFAULT_RECV_SIZE = KBytes(16);

   // define default listen() backlog size
   constexpr int SOCKET_DEFAULT_LISTEN_BACKLOG = 512;

   // define how long epoll should wait in milliseconds before returning
   constexpr int SOCKET_DEFAULT_EPOLL_WAIT_TIMEOUT_MS = 10;

   class socket_t
   {
      SOCKET handle_{ INVALID_SOCKET };
      SSL* ssl_{};
      SSL_CTX* ctx_{};
      uid_t uid_{};
      socket_mode_t mode_{ socket_mode_t::blocking };
      socket_state_t state_{ socket_state_t::idle };
      unsigned* ref_count_{};
      timer_t send_timer_;
      timer_t recv_timer_;

   public:
      // create a TCP socket
      socket_t() noexcept
         : ref_count_(new unsigned(1))
      {}


      // create a TLS or TCP socket
      explicit socket_t(tls::context_t& ctx) noexcept
         : ctx_{ ctx() }
         , ref_count_(new unsigned(1))
      {}

      ~socket_t() noexcept
      {
         close();
      }

      socket_t(const socket_t& other) noexcept
         : handle_{ other.handle_ }
         , ssl_{ other.ssl_ }
         , ctx_{ other.ctx_ }
         , uid_{ other.uid_ }
         , mode_{ other.mode_ }
         , state_{ other.state_ }
         , ref_count_{ other.ref_count_}
         , send_timer_{ other.send_timer_ }
         , recv_timer_{ other.recv_timer_ }
      {
         if (ref_count_)
         {
            ++(*ref_count_);
         }
      }

      socket_t(socket_t&& other) noexcept
         : handle_{ other.handle_ }
         , ssl_{ other.ssl_ }
         , ctx_{ other.ctx_ }
         , uid_{ other.uid_ }
         , mode_{ other.mode_ }
         , state_{ other.state_ }
         , ref_count_{ other.ref_count_ }
         , send_timer_{ other.send_timer_ }
         , recv_timer_{ other.recv_timer_ }
      {
         other.handle_ = INVALID_SOCKET;
         other.ssl_ = nullptr;
         other.ctx_ = nullptr;
         other.uid_ = 0;
         other.mode_ = socket_mode_t::blocking;
         other.state_ = socket_state_t::idle;
         other.ref_count_ = nullptr;
      }

      socket_t& operator=(const socket_t& other) noexcept
      {
         if (this != &other)
         {
            close();
            handle_ = other.handle_;
            ssl_ = other.ssl_;
            ctx_ = other.ctx_;
            uid_ = other.uid_;
            mode_ = other.mode_;
            state_ = other.state_;
            ref_count_ = other.ref_count_;
            send_timer_ = other.send_timer_;
            recv_timer_ = other.recv_timer_;
            if (ref_count_)
            {
               ++(*ref_count_);
            }
         }
         return *this;
      }

      socket_t& operator=(socket_t&& other) noexcept
      {
         if (this != &other)
         {
            // cleanup
            close();
            // transfer resources
            handle_ = other.handle_;
            ssl_ = other.ssl_;
            ctx_ = other.ctx_;
            uid_ = other.uid_;
            mode_ = other.mode_;
            state_ = other.state_;
            ref_count_ = other.ref_count_;
            send_timer_ = other.send_timer_;
            recv_timer_ = other.recv_timer_;
            // invalidate other
            other.handle_ = INVALID_SOCKET;
            other.ssl_ = nullptr;
            other.ctx_ = nullptr;
            other.uid_ = 0;
            other.mode_ = socket_mode_t::blocking;
            other.state_ = socket_state_t::idle;
            other.ref_count_ = nullptr;
         }
         return *this;
      }

      SOCKET handle() const noexcept
      {
         return handle_;
      }

      uid_t uid() const noexcept
      {
         return uid_;
      }

      socket_mode_t mode() const noexcept
      {
         return mode_;
      }

      socket_state_t state() const noexcept
      {
         return state_;
      }

      long long send_elapsed_usecs() const noexcept
      {
         return send_timer_.elapsed();
      }

      long long recv_elapsed_usecs() const noexcept
      {
         return recv_timer_.elapsed();
      }

      void send_timer_reset() noexcept
      {
         send_timer_.reset();
      }

      void recv_timer_reset() noexcept
      {
         recv_timer_.reset();
      }

      const SSL* ssl() const noexcept
      {
         return ssl_;
      }

      SSL* ssl() noexcept
      {
         return ssl_;
      }

      const SSL_CTX* ctx() const noexcept
      {
         return ctx_;
      }

      SSL_CTX* ctx() noexcept
      {
         return ctx_;
      }

      bool verify_peer_certificate() const noexcept
      {
         struct X509_deleter
         {
            void operator()(X509* certificate) const
            {
               if (certificate)
               {
                  X509_free(certificate);
               }
            }
         };

         if (!ssl_) return true;
         if (state_ != socket_state_t::connected) return false;
         if (std::unique_ptr<X509, X509_deleter> peer_certificate(SSL_get_peer_certificate(ssl_)); peer_certificate)
         {
            return SSL_get_verify_result(ssl_) == X509_V_OK;
         }
         return false;
      }

     socket::status_t connect(const ip::address_t& server, socket_mode_t mode = socket_mode_t::blocking) noexcept
      {
        socket::status_t status = tcp_connect(server, mode);
         if (status.ok() && ssl_)
         {
            status = ssl_connect();
         }
         return status;
      }

      // if an error is returned, the connection is closed and resources released,
      // and the error condition only indicate a gracefull disconnection has not 
      // possible
      socket::status_t disconnect(socket_close_t how = socket_close_t::send) noexcept
      {
         socket::status_t tcp_status;
         socket::status_t ssl_status;
         if (state_ == socket_state_t::connected)
         {
            if (ssl_)
            {
               if (int ret = SSL_shutdown(ssl_); ret != 0)
               {
                  ssl_status = socket::status_t{ ssl_, ret };
               }
            }
            tcp_status = socket::status_t{ ::shutdown(handle_, static_cast<int>(how)) };
         }
         close();
         if (tcp_status.ok()) return ssl_status;
         return tcp_status;
      }

      socket::status_t listen(const ip::address_t& server, socket_mode_t mode = socket_mode_t::blocking, int backlog = SOCKET_DEFAULT_LISTEN_BACKLOG) noexcept
      {
         socket::status_t status;
         if (state_ != socket_state_t::idle) return socket::status_t{ WSAEALREADY };
         if (status = create(server.family()); status.ok())
         {
            return status;
         }
         if (status = socket::status_t(::bind(handle_, server.address(), server.length())); status.ok())
         {
            if (status = socket::status_t(::listen(handle_, backlog)); status.ok())
            {
               if (status = set_blocking(mode); status.ok())
               {
                  generate_uid();
                  state_ = socket_state_t::listening;
               }
            }
         }
         if (status.nok())
         {
            close();
         }
         return status;
      }

      socket::status_t accept(socket_t& client, socket_mode_t mode = socket_mode_t::blocking) noexcept
      {
         socket::status_t status = tcp_accept(client, mode);
         if (status.ok() && state_ == socket_state_t::accepting)
         {
            int ret{ SSL_accept(client.ssl_) };
            if (ret <= 0)
            {
               if (status = socket::status_t{ client.ssl_, ret }; status.ok())
               {
                  client.state_ = socket_state_t::connected;
               }
               if (!status.ok() && !status.would_block())
               {
                  close();
               }
            }
            else
            {
               state_ = socket_state_t::listening;
            }
         }
         return status;
      }

      socket::status_t send(const char* buffer, size_t len, size_t& index, size_t& bytes_sent) noexcept
      {
         socket::status_t status;
         bytes_sent = 0;
         if (index >= len) return status;
         status = ssl_ ? ssl_send(buffer + index, (len - index), bytes_sent) 
                       : tcp_send(buffer + index, (len - index), bytes_sent);
         index += bytes_sent;
         return status;
      }

      // T should be a containers with data() and size() methods such as 
      // std::span, std::array, std::string_view, std::string and std::vector
      template <DataSizeContainer T>
      socket::status_t send(const T& buffer, size_t& index, size_t& bytes_sent) noexcept
      {
         return send(buffer.data(), buffer.size(), index, bytes_sent);
      }

      socket::status_t recv(char* buffer, size_t len, size_t& bytes_received) noexcept
      {
         bytes_received = 0;
         return ssl_ ? ssl_recv(buffer, len, bytes_received)
                     : tcp_recv(buffer, len, bytes_received);
      }

      // T should be a containers with data(), size() and resize() methods 
      // such as std::string and std::vector
      template <size_t S = SOCKET_DEFAULT_RECV_SIZE, DataSizeResizeContainer T>
      socket::status_t recv(T& buffer, size_t& bytes_received) noexcept
      {
         std::array<typename T::value_type, S> temp;
         socket::status_t status = recv(temp.data(), temp.size(), bytes_received);
         if (bytes_received > 0)
         {
            std::copy(temp.begin(), temp.begin() + bytes_received, std::back_inserter(buffer));
         }
         return status;
      }

      // timeout_us specifies how long wait will wait until an event occurs
      // SOCKET_WAIT_FOREVER to block until an event occurs
      // SOCKET_WAIT_NEVER to return immediately after checking
      // greater than zero value sets milli-seconds to wait until event occurs
      // wait succeeds:
      //   status_t::ok() == true, status_t::nok() == false, status_t::would_block() == false
      //
      // wait timeout
      //    status_t::ok() == false, status_t::nok() == true, status_t::would_block() == true
      // 
      // wait fails
      //    status_t::ok() == false, status_t::nok() == true, status_t::would_block() == false
      socket::status_t wait_event(socket_event_t event, wait_timeout_t timeout_ms = SOCKET_WAIT_NEVER) noexcept
      {
         using enum socket_event_t;
         int connect_flags = (POLLHUP | POLLERR | POLLWRNORM);
         WSAPOLLFD fdset;
         fdset.fd = handle_;
         fdset.revents = 0;
         fdset.events = set_events(event);
         int ret = WSAPoll(&fdset, 1, timeout_ms);
         if (ret == 0)
         {
            return socket::status_t{ WSAEWOULDBLOCK, get_code(event) };
         }
         if (ret == SOCKET_ERROR)
         {
            return socket::status_t{ last_error(), get_code(event) };
         }
         if (event == connect_ready && (fdset.revents & connect_flags) == connect_flags)
         {
            return socket::status_t(WSAECONNREFUSED);
         }
         if (event == connect_ready && fdset.events & POLLWRNORM)
         {
            return socket::status_t{};
         }
         return (fdset.events & (POLLHUP | POLLRDNORM | POLLWRNORM)) ? socket::status_t{} : socket::status_t{ WSAEWOULDBLOCK, get_code(event) };
      }

   private:
      static int last_error() noexcept
      {
         return ::WSAGetLastError();
      }

      status_code_t get_code(socket_event_t event) const noexcept
      {
         using enum socket_event_t;
         switch (event)
         {
            case recv_ready: return status_code_t::want_read;
            case send_ready: return status_code_t::want_write;
            case connect_ready: return status_code_t::want_write;
            case accept_ready: return status_code_t::want_read;
         }
         return status_code_t::want_read;
      }

      void generate_uid() noexcept
      {
         static std::atomic_uint64_t counter{ 1 };
         uid_ = counter.fetch_add(1, std::memory_order_relaxed);
      }

      short set_events(socket_event_t event) const noexcept
      {
         using enum socket_event_t;
         return (event == recv_ready || event == accept_ready) ? POLLRDNORM : POLLWRNORM;
      }

      socket::status_t set_blocking(socket_mode_t mode) noexcept
      {
         u_long um = (mode == socket_mode_t::nonblocking) ? 1 : 0;
         socket::status_t status{ ioctlsocket(handle_, FIONBIO, &um) };
         mode_ = status.ok() ? mode : mode_;
         return status;
      }

      socket::status_t create(int family) noexcept
      {
         handle_ = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
         if (handle_ == INVALID_SOCKET)
         {
            return socket::status_t{ last_error() };
         }
         if (ctx_)
         {
            ssl_ = SSL_new(ctx_);
         }
         state_ = socket_state_t::created;
         mode_ = socket_mode_t::blocking;
         return socket::status_t();
      }

      socket::status_t close() noexcept
      {
         socket::status_t status;
         if (ref_count_)
         {
            if (*ref_count_ == 1)
            {
               if (handle_ != INVALID_SOCKET)
               {
                  status = socket::status_t{ ::closesocket(handle_) };
                  handle_ = INVALID_SOCKET;
                  uid_ = 0;
                  mode_ = socket_mode_t::blocking;
                  state_ = socket_state_t::idle;
               }
               if (ssl_)
               {
                  SSL_free(ssl_);
                  ssl_ = nullptr;
               }
               delete ref_count_;
               ref_count_ = nullptr;
            }
            else
            {
               --(*ref_count_);
            }
         }
         return status;
      }

      socket::status_t tcp_connect(const ip::address_t& server, socket_mode_t mode) noexcept
      {
         using enum socket_state_t;
         if (state_ == connecting) return socket::status_t{};
         if (state_ != idle) return socket::status_t{ WSAEALREADY };
         socket::status_t status;
         if (create(server.family()); status.ok())
         {
            if (status = socket::status_t{ ::connect(handle_, server.address(), server.length()) }; status.ok())
            {
               status = set_blocking(mode);
            }
         }
         if (status.nok())
         {
            close();
            return status;
         }
         generate_uid();
         state_ = connected;
         if (ssl_)
         {
            SSL_set_fd(ssl_, static_cast<int>(handle_));
            state_ = connecting;
         }
         send_timer_.reset();
         recv_timer_.reset();
         return status;
      }

      socket::status_t ssl_connect() noexcept
      {
         if (int ret{ SSL_connect(ssl_) }; ret <= 0)
         {
            socket::status_t status{ ssl_, ret };
            if (!status.ok() && !status.would_block())
            {
               close();
            }
            return status;
         }
         state_ = socket_state_t::connected;
         return socket::status_t{};
      }

      socket::status_t tcp_accept(socket_t& client, socket_mode_t mode, ip::address_t& address) noexcept
      {
         socket::status_t status;
         if (state_ == socket_state_t::accepting) return status;
         socket_t socket;
         sockaddr name{};
         socklen_t namelen{};
         if (socket.handle_ = ::accept(handle_, &name, &namelen); socket.handle_ == INVALID_SOCKET)
         {
            return socket::status_t{ last_error() };
         }
         if (status = socket.set_blocking(mode); status.nok())
         {
            socket.close();
            return status;
         }
         socket.mode_ = mode;
         socket.state_ = socket_state_t::connected;
         socket.generate_uid();
         address = ip::address_t(name, namelen);
         if (ssl_)
         {
            socket.ctx_ = ctx_;
            socket.ssl_ = SSL_new(ctx_);
            SSL_set_fd(socket.ssl_, static_cast<int>(socket.handle_));
         }
         client = socket;
         client.send_timer_.reset();
         client.recv_timer_.reset();
         return status;
      }

      socket::status_t tcp_accept(socket_t& client, socket_mode_t mode) noexcept
      {
         ip::address_t address;
         return tcp_accept(client, mode, address);
      }

      socket::status_t tcp_send(const char* buffer, size_t len, size_t& bytes_sent) noexcept
      {
         if (state_ != socket_state_t::connected) return socket::status_t{ WSAENOTCONN };
         int ret = ::send(handle_, buffer, static_cast<int>(len), 0);
         if (ret == SOCKET_ERROR)
         {
            return socket::status_t{ last_error(), status_code_t::want_write };
         }
         bytes_sent = static_cast<size_t>(ret);
         send_timer_.reset();
         return socket::status_t();
      }

      socket::status_t ssl_send(const char* buffer, size_t len, size_t& bytes_sent) noexcept
      {
         if (state_ != socket_state_t::connected) return socket::status_t{ WSAENOTCONN };
         if (int ret = SSL_write_ex(ssl_, buffer, len, &bytes_sent); ret <= 0)
         {
            return socket::status_t{ ssl_, ret };
         }
         send_timer_.reset();
         return socket::status_t();
      }

      // if status_t::ok() == true and bytes_received == 0, then peer closing connection
      socket::status_t tcp_recv(char* buffer, size_t len, size_t& bytes_received) noexcept
      {
         if (state_ != socket_state_t::connected) return socket::status_t{ WSAENOTCONN };
         int ret = ::recv(handle_, buffer, static_cast<int>(len), 0);
         bytes_received = 0;
         if (ret == SOCKET_ERROR)
         {
            return socket::status_t{ last_error(), status_code_t::want_read };
         }
         else if (ret == 0)
         {
            return socket::status_t{ 0, status_code_t::closing };
         }
         bytes_received = static_cast<size_t>(ret);
         recv_timer_.reset();
         return socket::status_t();
      }

      socket::status_t ssl_recv(char* buffer, size_t len, size_t& bytes_received) noexcept
      {
         if (state_ != socket_state_t::connected) return socket::status_t{ WSAENOTCONN };
         if (int ret = SSL_read_ex(ssl_, buffer, len, &bytes_received); ret <= 0)
         {
            return socket::status_t{ ssl_, ret};
         }
         recv_timer_.reset();
         return socket::status_t();
      }

   }; // class socket_t

   namespace utility {

#if defined(XPLAT_OS_WIN)
      /**********************************************************************\
      *
      *  winsock_init_t
      *  Windows socket initialization. DO NOT INSTANTIATE THIS CLASS
      *
      \**********************************************************************/

      class winsock_init_t
      {
         mutable WSADATA wd_;
         int code_{};

      public:
         winsock_init_t() noexcept
         {
            code_ = ::WSAStartup(MAKEWORD(2, 2), &wd_);
         }

         ~winsock_init_t() noexcept
         {
            ::WSACleanup();
         }

         int code() const noexcept
         {
            return code_;
         }

         winsock_init_t(const winsock_init_t&) = delete;
         winsock_init_t(winsock_init_t&&) = delete;
         winsock_init_t& operator=(const winsock_init_t&) = delete;
         winsock_init_t& operator=(winsock_init_t&&) = delete;
      }; // class socket_init_t

      inline const static winsock_init_t socket_startup_;
#endif

      class openssl_t
      {
         inline static std::atomic_uint32_t counter_{ 0 };

      public:
         // no copy or move allowed
         openssl_t(const openssl_t&) = delete;
         openssl_t(openssl_t&&) noexcept = delete;
         openssl_t& operator=(const openssl_t&) = delete;
         openssl_t& operator=(openssl_t&&) noexcept = delete;

         openssl_t() noexcept
         {
            initialize();
         }

         ~openssl_t() noexcept
         {
            cleanup();
         }

      private:
         static void initialize() noexcept
         {
            if (counter_.fetch_add(1, std::memory_order_relaxed) == 0)
            {
               SSL_library_init();
               ERR_load_crypto_strings();
               SSL_load_error_strings();
               OpenSSL_add_all_algorithms();
            }
         }

         static void cleanup() noexcept
         {
            if (counter_.fetch_sub(1, std::memory_order_relaxed) == 1)
            {
               ERR_free_strings();
               EVP_cleanup();
               CRYPTO_cleanup_all_ex_data();
               SSL_COMP_free_compression_methods();
            }
         }
      }; // class openssl_t

      inline const static openssl_t openssl_startup_;

   } // namespace utility


} // namespace netptune