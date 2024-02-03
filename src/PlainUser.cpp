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

using boost::asio::ip::tcp;

class PlainUser
  : public User, public std::enable_shared_from_this<PlainUser>
{
std::weak_ptr<PlainUser> self;
public:

  PlainUser(tcp::socket socket)
    : socket_(std::move(socket))
    , deadline(socket_.get_executor())
  {
  }
  virtual ~PlainUser() { deadline.cancel(); };
  
  std::string ip()
  {
    try {
	  if (socket_.is_open())
        return socket_.remote_endpoint().address().to_string();
    } catch (boost::system::system_error &e) {
	  std::cout << "ERROR getting IP in plain mode" << std::endl;
    }
    return "127.0.0.0";
  }

  void start()
  {
	if (config["maxUsers"].as<long unsigned int>() <= Users.size()) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "The server has reached maximum number of connections."));
		Close();
	} else if (Server::CheckClone(ip()) == true) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You have reached the maximum number of clones."));
		Close();
	} else if (Server::CheckDNSBL(ip()) == true) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "Your IP is in our DNSBL lists."));
		Close();
    } else if (Server::CheckThrottle(ip()) == true) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You connect too fast, wait 30 seconds to try connect again."));
		Close();
	} else if (OperServ::IsGlined(ip()) == true) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are G-Lined. Reason: %s", OperServ::ReasonGlined(ip()).c_str()));
		Close();
	} else if (OperServ::IsTGlined(ip()) == true) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You are Timed G-Lined. Reason: %s", OperServ::ReasonTGlined(ip()).c_str()));
		Close();
	} else if (OperServ::CanGeoIP(ip()) == false) {
		SendAsServer("465 ZeusiRCd :" + Utils::make_string("", "You can not connect from your country."));
		Close();
	} else {
		do_read();
		deadline.cancel(); 
		deadline.expires_from_now(boost::posix_time::seconds(10)); 
		deadline.async_wait(std::bind(&PlainUser::check_deadline, this, std::placeholders::_1));
		mHost = ip();
		bPing = time(0);
		bSendQ = time(0);
	}
  }

  void Close() override
  {
	boost::system::error_code ignored_error;
	deadline.cancel();
	if (socket_.is_open() == false) return;
	socket_.cancel(ignored_error);
	socket_.close(ignored_error);
	if (auto ptr = self.lock()) {
        // Eliminar el objeto usando ptr
        ptr.reset(); // Libera la referencia compartida
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
	        deadline.async_wait(std::bind(&PlainUser::check_ping, this, std::placeholders::_1));
	    }
	}
  }
  
  void check_ping(const boost::system::error_code &e) {
	if (!e && socket_.is_open() == true) {
		if (bPing + 200 < time(0))
			Exit(true);
        else {
			deliver("PING :" + config["serverName"].as<std::string>());
			deadline.cancel();
			deadline.expires_from_now(boost::posix_time::seconds(20));
			deadline.async_wait(std::bind(&PlainUser::check_ping, this, std::placeholders::_1));
		}
	}
  }

void deliver(const std::string &msg) override {
    try {
        if (!socket_.is_open() || quit) {
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

void prior(const std::string &msg) override {
    if (!socket_.is_open() || quit) {
        throw std::runtime_error("Socket is not open");
        Exit(false);
    }

  std::string mensaje = msg + "\r\n";
  auto self(shared_from_this());  // Ensure lifetime for callback
  boost::asio::async_write(socket_,
    boost::asio::buffer(mensaje),
    [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      if (ec) {
        // Handle write error gracefully (e.g., log, retry, notify)
        Exit(false);  // Or consider more specific actions
      }
    });
}

private:
  void do_read()
  {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, mBuffer, "\n",
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
          }
          else
          {
            Exit(false);
          }
        });
  }
  
void do_write() {
  if (queue.empty()) return;

  auto self(shared_from_this());  // Ensure lifetime for callback
  boost::asio::async_write(socket_,
    boost::asio::buffer(queue.front().data(), queue.front().length()),
    [this, self](boost::system::error_code ec, std::size_t /*length*/) {
      std::lock_guard<std::mutex> lock(mtx);  // Lock only for queue operations
      if (!ec) {
        queue.pop();  // Remove successfully written item
        do_write();  // Continue writing if more items exist
      } else {
        Exit(false);  // Handle write error gracefully
      }
    });
}

  tcp::socket socket_;
  boost::asio::deadline_timer deadline;
  boost::asio::streambuf mBuffer;
  std::queue<std::string> queue;
  std::mutex mtx;
};

void Listen::do_accept() {
  // Iniciar la aceptación asincrónica en el executor del pool
  acceptor_.async_accept(io_context_pool_.get_executor(),
      [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
             std::make_shared<PlainUser>(std::move(socket))->start();
          }
          do_accept();
      });
}
