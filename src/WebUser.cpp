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

using boost::asio::ip::tcp;
typedef boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>> web_socket;

class WebUser
  : public User, public std::enable_shared_from_this<WebUser>
{
public:
  WebUser(boost::asio::io_context& io_context,
      boost::asio::ssl::context& context)
    : socket_(io_context, context)
    , deadline(boost::asio::system_executor())
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
    return "127.0.0.0";
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
	  boost::system::error_code ignored_error;
	  deadline.cancel();
	  if (socket_.next_layer().next_layer().is_open() == false) return;
	  socket_.next_layer().next_layer().cancel(ignored_error);
	  socket_.next_layer().next_layer().close(ignored_error);
  }
  
  void check_deadline(const boost::system::error_code &e)
  {
	if (!e && bSentNick == false)
	  Exit(true);
	else {
	  deadline.expires_from_now(boost::posix_time::seconds(30)); 
	  deadline.async_wait(std::bind(&WebUser::check_ping, this, std::placeholders::_1));
	}
  }
  
  void check_ping(const boost::system::error_code &e) {
	if (!e) {
	  if (bPing + 200 < time(0)) {
		Exit(true);
	  } else {
		deliver("PING :" + config["serverName"].as<std::string>());
		deadline.expires_from_now(boost::posix_time::seconds(30));
		deadline.async_wait(std::bind(&WebUser::check_ping, this, std::placeholders::_1));
	  }
	}
  }

  void deliver(const std::string msg) override
  {
	if (socket_.is_open() == false)
		Exit(false);
    else
    {
		bool write_in_progress = !queue.empty();
		queue.push(msg);
		if (!write_in_progress)
		{
		  do_write();
		}
	}
  }

  void prior(const std::string msg) override
  {
	if (socket_.is_open() == false)
		Exit(false);
    else
    {
    socket_.async_write(boost::asio::buffer(msg),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (ec)
            Exit(true);
        });
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

  void do_write()
  {
    socket_.async_write(boost::asio::buffer(queue.front().data(),
          queue.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
			mtx.lock();
            queue.pop();
            mtx.unlock();
            if (!queue.empty())
            {
              do_write();
            }
          }
          else
          {
            Exit(false);
          }
        });
  }

  web_socket socket_;
  boost::asio::deadline_timer deadline;
  boost::asio::streambuf mBuffer;
  bool handshake = false;
  std::queue <std::string> queue;
  std::mutex mtx;
};

void ListenWSS::do_accept()
{
	boost::asio::ssl::context context_(boost::asio::ssl::context::sslv23);
	context_.set_options(boost::asio::ssl::context::single_dh_use);
	context_.use_certificate_chain_file("server.pem");
	context_.use_private_key_file("server.key", boost::asio::ssl::context::pem);
	context_.use_tmp_dh_file("dh.pem");
	auto new_session = std::make_shared<WebUser>(io_context_pool_.get_io_context(), context_);
	acceptor_.async_accept(new_session->socket_.next_layer().next_layer(),
					   boost::bind(&ListenWSS::handle_handshake,   this,   new_session,  boost::asio::placeholders::error));
}

void ListenWSS::handle_handshake(const std::shared_ptr<WebUser> new_session, const boost::system::error_code& error) {
	if (!error)
		new_session->socket_.next_layer().async_handshake(boost::asio::ssl::stream_base::server, boost::bind(&ListenWSS::handle_accept, this, new_session, boost::asio::placeholders::error));
	else
		new_session->Close();
}

void
ListenWSS::handle_accept(const std::shared_ptr<WebUser> new_session,
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
	// Accept another connection
	do_accept();
}
