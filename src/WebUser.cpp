/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *FExit
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ZeusiRCd.h"
#include "services.h"

#include <cstdlib>
#include <queue>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/thread_pool.hpp>

using boost::asio::ip::tcp;
typedef boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> web_socket;

class WebUser
  : public User, public std::enable_shared_from_this<WebUser>
{
public:
  WebUser(boost::asio::io_context& io,
        boost::asio::ssl::context& context)
    : socket_(io.get_executor(), context)  // Usar executor del pool
    , deadline(io.get_executor())         // Usar executor del pool
    {
    }
  virtual ~WebUser() { deadline.cancel(); };

  std::string ip()
  {
    try {
	  if (socket_.next_layer().next_layer().is_open())
        return socket_.next_layer().next_layer().remote_endpoint().address().to_string();
    } catch (boost::system::system_error &e) {
	  std::cout << "ERROR getting IP in plain mode" << std::endl;
    }
    return "0.0.0.0";
  }

  void start()
  {
    deadline.cancel(); 
	deadline.expires_from_now(boost::posix_time::seconds(10)); 
	deadline.async_wait(std::bind(&WebUser::check_deadline, this, std::placeholders::_1));
	mHost = ip();
	setMode('w', true);
	bPing = time(0);
	bSendQ = time(0);
    do_read();
  }

  void on_accept(boost::beast::error_code ec)
  {
	if (!ec)
	{
		handshake = true;
		do_read();
	} else
		Exit(true);
  }

  void Close() override
  {
    try {
        // Cancelar temporizadores y operaciones pendientes
        deadline.cancel();

        // Cerrar el socket, manejando posibles errores
        boost::system::error_code ec;
        socket_.next_layer().next_layer().cancel(ec);

        // Si se produce un error al cerrar el socket, registrarlo
        if (ec) {
            std::cerr << "Error cancel socket: " << ec.message() << std::endl;
            throw boost::system::system_error(ec);
        }
        // Cerrar el socket, lanzando excepciones en caso de error
        socket_.next_layer().next_layer().close(ec);

        if (ec) {
            std::cerr << "Error closing socket: " << ec.message() << std::endl;
            throw boost::system::system_error(ec);
        }
    } catch (const boost::system::system_error& e) {
        // Registrar el error de forma adecuada
        std::cerr << "Error closing socket: " << e.what() << std::endl;
    }
  }
  
  void check_deadline(const boost::system::error_code &e)
  {
	if (!e) {
	    if (!bSentNick)
	        Exit(true);
	    else {
	        deadline.cancel();
	        deadline.expires_from_now(boost::posix_time::seconds(20)); 
	        deadline.async_wait(std::bind(&WebUser::check_ping, this, std::placeholders::_1));
	    }
	} else
	    Exit(false);
  }

  void check_ping(const boost::system::error_code &e) {
	if (!e) {
		if (bPing + 200 < time(0))
			Exit(true);
        else {
			deliver("PING :" + config["serverName"].as<std::string>());
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(20));
			deadline.async_wait(std::bind(&WebUser::check_ping, this, std::placeholders::_1));
		}
	} else
	    Exit(false);
  }

  bool invalidChar (char c) 
  {  
    return !(c>=0 && c <128);   
  }

  std::string stripUnicode(std::string str) 
  {
	  std::string buf = "";
	  for (unsigned int i = 0; i < str.length(); i++)
	  {
		if (invalidChar(str[i]) == true)
			buf += "?";
		else
			buf += str[i];
	  }
	  return buf;
  }

  void deliver(const std::string &msg) override
  {
	if (socket_.next_layer().next_layer().is_open() == false) {
		throw std::runtime_error("Socket is not open");
        Exit(false);
	}
    else
    {
		boost::asio::post(
				socket_.get_executor(),
				boost::beast::bind_front_handler(
					&WebUser::on_send,
					shared_from_this(),
					stripUnicode(msg)));
	}
  }

  void prior(const std::string &msg) override
  {
	if (socket_.next_layer().next_layer().is_open() == false)
		Exit(false);
    else
    {
	    boost::asio::post(
		socket_.get_executor(),
		boost::beast::bind_front_handler(
			&WebUser::on_send,
			shared_from_this(),
			stripUnicode(msg)));
   }
  }

  void do_read()
  {
	auto self(shared_from_this());
    socket_.async_read(mBuffer,
        [this, self](boost::system::error_code ec, std::size_t bytes)
        {
		  if (!handshake)
		  {
			socket_.async_accept(
                boost::beast::bind_front_handler(
                    &WebUser::on_accept,
                    shared_from_this()));
          }
          else if (!ec)
          {
            std::string message;
			std::istream istream(&mBuffer);
			std::getline(istream, message);
            message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
            message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
            message.erase(std::remove(message.begin(), message.end(), '\t'), message.end());
            message.erase(std::remove(message.begin(), message.end(), '\0'), message.end());
            
            User::Parse(message);
            
            if (bSendQ + 30 > time(0))
              SendQ += bytes;
            else {
              SendQ = 0;
              bSendQ = time(0);
			}
				
            if (SendQ > 1024*3) {
              Exit(true);
			}

			do_read();
          }
          else
          {
            Exit(false);
          }
        });
  }

	void on_send(std::string const ss)
	{
		queue.push_back(ss);

		if(queue.size() > 1)
			return;

		socket_.async_write(
			boost::asio::buffer(queue.front()),
			boost::beast::bind_front_handler(
				&WebUser::on_write,
				shared_from_this()));
	}

	void on_write(boost::beast::error_code ec, std::size_t)
	{
		if (ec)
			Exit(false);

		queue.erase(queue.begin());

		if(!queue.empty())
			socket_.async_write(
				boost::asio::buffer(queue.front()),
				boost::beast::bind_front_handler(
					&WebUser::on_write,
					shared_from_this()));
	}

  web_socket socket_;
  boost::asio::deadline_timer deadline;
  boost::asio::streambuf mBuffer;
  bool handshake = false;
  std::vector<std::string> queue;
};

void ListenWSS::do_accept()
{
	boost::asio::ssl::context context_(boost::asio::ssl::context::sslv23);
	context_.set_options(boost::asio::ssl::context::single_dh_use |
						boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::verify_none);
	context_.use_certificate_chain_file("server.pem");
	context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	context_.use_tmp_dh_file("dh.pem");
	auto new_session = std::make_shared<WebUser>(io_context_pool_.get_io_context(), context_);
	acceptor_.async_accept(new_session->socket_.next_layer().next_layer(),
					   boost::bind(&ListenWSS::handle_handshake, this, new_session, boost::asio::placeholders::error));
}

void ListenWSS::handle_handshake(std::shared_ptr<WebUser> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->deadline.expires_from_now(boost::posix_time::seconds(10));
		new_session->deadline.async_wait([new_session](const boost::system::error_code& error) {
			if (!error)
				new_session->Close();
		});
		new_session->socket_.next_layer().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ListenWSS::handle_accept, this, new_session, boost::asio::placeholders::error));
	} else {
		new_session->Close();
	}
	do_accept();
}

void
ListenWSS::handle_accept(std::shared_ptr<WebUser> new_session,
  const boost::system::error_code& error)
{
	if(!error)
	{
		if (config["maxUsers"].as<long unsigned int>() <= Users.size()) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections."));
			new_session->Close();
		} else if (Server::CheckClone(new_session->ip()) == true) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones."));
			new_session->Close();
		} else if (Server::CheckDNSBL(new_session->ip()) == true) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists."));
			new_session->Close();
		} else if (Server::CheckThrottle(new_session->ip()) == true) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again."));
			new_session->Close();
		} else if (OperServ::IsGlined(new_session->ip()) == true) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(new_session->ip()).c_str()));
			new_session->Close();
		} else if (OperServ::IsTGlined(new_session->ip()) == true) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are Timed G-Lined. Reason: %s", OperServ::ReasonTGlined(new_session->ip()).c_str()));
			new_session->Close();
		} else if (OperServ::CanGeoIP(new_session->ip()) == false) {
			new_session->SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country."));
			new_session->Close();
		} else {
			new_session->start();
		}
	}
}
