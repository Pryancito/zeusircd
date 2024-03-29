/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#define ZASIO
#include "ZeusiRCd.h"
#include "services.h"

#include <cstdlib>
#include <queue>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;
typedef boost::asio::ssl::stream<tcp::socket> ssl_socket;

class SSLUser
  : public User, public std::enable_shared_from_this<SSLUser>
{
public:
    SSLUser(boost::asio::io_context& io,
        boost::asio::ssl::context& context)
    : socket_(io.get_executor(), context)  // Usar executor del pool
    , deadline(io.get_executor())         // Usar executor del pool
    {
    }

  virtual ~SSLUser() { deadline.cancel(); };
  
  std::string ip()
  {
    try {
	  if (socket_.lowest_layer().is_open())
        return socket_.lowest_layer().remote_endpoint().address().to_string();
    } catch (boost::system::system_error &e) {
	  std::cout << "ERROR getting IP in plain mode" << std::endl;
    }
    return "0.0.0.0";
  }

  void start()
  {
	deadline.cancel();
	deadline.expires_from_now(boost::posix_time::seconds(10)); 
	deadline.async_wait(std::bind(&SSLUser::check_deadline, this, std::placeholders::_1));
	mHost = ip();
	setMode('z', true);
	bPing = time(0);
	bSendQ = time(0);
    do_read();
  }
  
  void Close() override
  {
    try {
        // Cancelar temporizadores y operaciones pendientes
        deadline.cancel();

        // Cerrar el socket, manejando posibles errores
        boost::system::error_code ec;
        socket_.lowest_layer().cancel(ec);

        // Si se produce un error al cerrar el socket, registrarlo
        if (ec) {
            std::cerr << "Error cancel socket: " << ec.message() << std::endl;
            throw boost::system::system_error(ec);
        }
        // Cerrar el socket, lanzando excepciones en caso de error
        socket_.lowest_layer().close(ec);

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
	        deadline.async_wait(std::bind(&SSLUser::check_ping, this, std::placeholders::_1));
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
			deadline.async_wait(std::bind(&SSLUser::check_ping, this, std::placeholders::_1));
		}
	} else
	    Exit(false);
  }

  void deliver(const std::string &msg) override
  {
    try {
        if (!socket_.lowest_layer().is_open()) {
            throw std::runtime_error("Socket is not open");
            Exit(false);
        }

//        std::lock_guard<std::mutex> lock(mtx); // Use RAII for lock management
        bool write_in_progress = !queue.empty();
        queue.push(msg + "\r\n");

        if (write_in_progress) {
            return;  // Avoid redundant calls to do_write
        }

        do_write();
    } catch (const std::exception& e) {
        // Handle exceptions gracefully (e.g., log the error, attempt reconnect)
        std::cerr << "Error in deliver: " << e.what() << std::endl;
        // Consider retry logic or error reporting mechanisms
    }
  }

  void prior(const std::string &msg) override
  {
	if (socket_.lowest_layer().is_open() == false) {
        throw std::runtime_error("Socket is not open");
	    Exit(false);
	}
    else
    {
		std::string mensaje = msg + "\r\n";
		boost::asio::async_write(socket_,
        boost::asio::buffer(mensaje),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
			if (ec)
				Exit(false);
		});
	}
  }

  void do_read()
  {
	auto self(shared_from_this());
    boost::asio::async_read_until(socket_, mBuffer, '\n',
        [this, self](boost::system::error_code ec, std::size_t bytes)
        {
          if (!ec)
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
          } else {
			Exit(false);
		  }
        });
  }

  void do_write()
  {
  if (queue.empty()) return;

  auto self(shared_from_this());  // Ensure lifetime for callback
  boost::asio::async_write(socket_,
    boost::asio::buffer(queue.front().data(), queue.front().length()),
    [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (!ec) {
        queue.pop();  // Remove successfully written item
        do_write();  // Continue writing if more items exist
      } else {
        Exit(false);  // Handle write error gracefully
      }
    });
  }

  ssl_socket socket_;
  boost::asio::deadline_timer deadline;
  boost::asio::streambuf mBuffer;
  std::queue<std::string> queue;
  std::mutex mtx;
};

void ListenSSL::start_accept()
{
  boost::asio::ssl::context context_(boost::asio::ssl::context::sslv23);
  context_.set_options(boost::asio::ssl::context::single_dh_use |
						boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::verify_none);
  context_.use_certificate_chain_file("server.pem");
  context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
  context_.use_tmp_dh_file("dh.pem");
  auto new_session = std::make_shared<SSLUser>(io_context_pool_.get_io_context(), context_);
  acceptor_.async_accept(new_session->socket_.lowest_layer(),
                          boost::bind(&ListenSSL::handle_handshake, this, new_session, boost::asio::placeholders::error));
}

void ListenSSL::handle_handshake(std::shared_ptr<SSLUser> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->deadline.expires_from_now(boost::posix_time::seconds(10));
		new_session->deadline.async_wait([new_session](const boost::system::error_code& error) {
			if (!error)
				new_session->Close();
		});
		new_session->socket_.async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ListenSSL::handle_accept, this, new_session, boost::asio::placeholders::error));
	} else {
		new_session->Close();
	}
	start_accept();
}

void ListenSSL::handle_accept(std::shared_ptr<SSLUser> new_session,
  const boost::system::error_code& error)
{
  if (!error)
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
