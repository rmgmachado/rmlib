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
#include "rmlib/socket.h"

using namespace rmlib;


const std::string echo_tcp_server_name{ "tcpbin.com" };
const std::string echo_tcp_server_numeric{ "45.79.112.203" };
const std::string echo_tcp_server_port{ "4242" };
const std::string echo_tcp_server_url{ "tcpbin.com:4242" };
const std::string echo_tcp_server_numeric_url{ "45.79.112.203:4242" };

const std::string echo_tls_server_name{ "tcpbin.com" };
const std::string echo_tls_server_numeric{ "45.79.112.203" };
const std::string echo_tls_server_port{ "4243" };
const std::string echo_tls_server_url{ "tcpbin.com:4243" };
const std::string echo_tls_server_numeric_url{ "45.79.112.203:4243" };

const std::string bogus_name("total_bogus_host_url");
const std::string bogus_port("10101");
const std::string bogus_url("total_bogus_host_url:10101");

TEST_CASE("Test socket::status_t class", "[status_t]")
{
	SECTION("Test status_t default constructor")
	{
		socket::status_t status;
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
	}
	SECTION("Test socket::status_t explicit constructor")
	{
		socket::status_t status(WSAEWOULDBLOCK);
		REQUIRE(!status.ok());
		REQUIRE(status.nok());
	}
}

TEST_CASE("Test functions", "[functions]")
{
	SECTION("Test ip::local_host_name() function")
	{
		std::string name;
		socket::status_t status = ip::local_host_name(name);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(name.length() > 0);
	}
	SECTION("Test ip::name_resolution() name and port")
	{
		ip::address_list_t address_list;
		socket::status_t status = ip::address_resolution(echo_tcp_server_name, echo_tcp_server_port, address_list);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(address_list.size() > 0);
	}
	SECTION("Test ip::name_resolution() name:port")
	{
		ip::address_list_t address_list;
		socket::status_t status = ip::address_resolution(echo_tcp_server_url, address_list);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(address_list.size() > 0);
	}
	SECTION("Test ipname_resolution() numeric and port")
	{
		ip::address_list_t address_list;
		socket::status_t status = ip::address_resolution(echo_tcp_server_numeric, echo_tcp_server_port, address_list);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(address_list.size() > 0);
	}
	SECTION("Test ipname_resolution() numeric:port")
	{
		ip::address_list_t address_list;
		socket::status_t status = ip::address_resolution(echo_tcp_server_numeric_url, address_list);
		REQUIRE(status.ok());
		REQUIRE(!status.nok());
		REQUIRE(address_list.size() > 0);
	}
	SECTION("Test ipname_resolution() failure")
	{
		ip::address_list_t address_list;
		socket::status_t status = ip::address_resolution(bogus_name, bogus_port, address_list);
		REQUIRE(!status.ok());
		REQUIRE(status.nok());
		REQUIRE(address_list.empty());
	}
}

ip::address_t server_address(const std::string& url) noexcept
{
	ip::address_list_t list;
	if (socket::status_t status = ip::address_resolution(url, list); status.ok() && !list.empty())
	{
		return list[0];
	}
	return ip::address_t{};
}

socket::status_t wait_event(socket_t& socket, socket::status_t status, wait_timeout_t timeout_ms = 100) noexcept
{
	socket_event_t event{ status.want_read() ? socket_event_t::recv_ready : socket_event_t::send_ready };
	return socket.wait_event(event, timeout_ms);
}

template <DataSizeContainer T>
socket::status_t send_msg(socket_t& socket, const T& msg, size_t& bytes_sent) noexcept
{
	socket::status_t status;
	size_t index{};
	bytes_sent = 0;
	while (status.ok() && index < msg.size())
	{
		size_t count{};
		if (status = socket.send(msg, index, count); status.nok())
		{
			if (status.would_block())
			{
				wait_event(socket, status);
				status.clear();
			}
		}
		bytes_sent += count;
	}
	return status;
}

template <size_t S = SOCKET_DEFAULT_RECV_SIZE, DataSizeResizeContainer T>
socket::status_t recv_msg(socket_t& socket, T& msg, size_t bytes_to_receive, size_t& bytes_received) noexcept
{
	socket::status_t status;
	bytes_received = 0;
	msg.clear();
	while (status.ok() && bytes_received < bytes_to_receive)
	{
		size_t count{};
		if (status = socket.recv<S, T>(msg, count); status.nok())
		{
			if (status.would_block())
			{
				wait_event(socket, status);
				status.clear();
			}
		}
		bytes_received += count;
	}
	return status;
}

TEST_CASE("Test TCP echo client - blocking", "[tcp-echo-cli-blk]")
{
	socket_t socket;
	socket::status_t status = socket.connect(server_address(echo_tcp_server_url));
	REQUIRE(status.ok());
	std::string send_buffer(4095, 'z' );
	send_buffer += "\n";
	size_t bytes_sent{};
	REQUIRE(send_msg(socket, send_buffer, bytes_sent).ok());
	REQUIRE(bytes_sent == send_buffer.size());
	std::string recv_buffer;
	size_t bytes_received{};
	REQUIRE(recv_msg(socket, recv_buffer, bytes_sent, bytes_received).ok());
	REQUIRE(bytes_sent == bytes_received);
	REQUIRE(send_buffer == recv_buffer);
	REQUIRE(socket.disconnect().ok());
}

TEST_CASE("Test TLS echo client - blocking", "[tls-echo-cli-blk]")
{
	tls::context_t ctx(tls::context_type_t::client);
	socket_t socket(ctx);
	socket::status_t status = socket.connect(server_address(echo_tls_server_url));
	REQUIRE(status.ok());
	std::string send_buffer(4095, 'z');
	send_buffer += "\n";
	size_t bytes_sent{};
	REQUIRE(send_msg(socket, send_buffer, bytes_sent).ok());
	REQUIRE(bytes_sent == send_buffer.size());
	std::string recv_buffer;
	size_t bytes_received{};
	REQUIRE(recv_msg(socket, recv_buffer, bytes_sent, bytes_received).ok());
	REQUIRE(bytes_sent == bytes_received);
	REQUIRE(send_buffer == recv_buffer);
	REQUIRE(socket.disconnect().ok());
}

TEST_CASE("Test TCP echo client - blocking - small buffer", "[tcp-echo-cli-blk-sml]")
{
	socket_t socket;
	socket::status_t status = socket.connect(server_address(echo_tcp_server_url));
	REQUIRE(status.ok());
	std::string send_buffer(4095, 'z');
	send_buffer += "\n";
	size_t bytes_sent{};
	REQUIRE(send_msg(socket, send_buffer, bytes_sent).ok());
	REQUIRE(bytes_sent == send_buffer.size());
	std::string recv_buffer;
	size_t bytes_received{};
	REQUIRE(recv_msg<256>(socket, recv_buffer, bytes_sent, bytes_received).ok());
	REQUIRE(bytes_sent == bytes_received);
	REQUIRE(send_buffer == recv_buffer);
	REQUIRE(socket.disconnect().ok());
}

TEST_CASE("Test TLS echo client - blocking - small buffer", "[tls-echo-cli-blk-sml]")
{
	tls::context_t ctx(tls::context_type_t::client);
	socket_t socket(ctx);
	socket::status_t status = socket.connect(server_address(echo_tls_server_url));
	REQUIRE(status.ok());
	std::string send_buffer(4095, 'z');
	send_buffer += "\n";
	size_t bytes_sent{};
	REQUIRE(send_msg(socket, send_buffer, bytes_sent).ok());
	REQUIRE(bytes_sent == send_buffer.size());
	std::string recv_buffer;
	size_t bytes_received{};
	REQUIRE(recv_msg<256>(socket, recv_buffer, bytes_sent, bytes_received).ok());
	REQUIRE(bytes_sent == bytes_received);
	REQUIRE(send_buffer == recv_buffer);
	REQUIRE(socket.disconnect().ok());
}
