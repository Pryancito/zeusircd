#include "mainframe.h"
#include "Config.h"
#include "module.h"
#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;
using namespace std;

class web
{
public:
  web(boost::asio::io_context& io_context,
      const std::string& server, const std::string& path)
    : resolver_(io_context),
      socket_(io_context)
  {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    resolver_.async_resolve(server, "http",
        boost::bind(&web::handle_resolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::results));
  }

private:
  void handle_resolve(const boost::system::error_code& err,
      const tcp::resolver::results_type& endpoints)
  {
    if (!err)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::async_connect(socket_, endpoints,
          boost::bind(&web::handle_connect, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_connect(const boost::system::error_code& err)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
          boost::bind(&web::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_write_request(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          boost::bind(&web::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        std::cout << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
        std::cout << "Response returned with status code ";
        std::cout << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&web::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        std::cout << &response_;

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&web::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&web::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
  }

  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
};

class WEB_UP : public Module
{
  public:
	std::thread *th;
	boost::asio::io_context ioc;
	boost::asio::deadline_timer deadline;
	WEB_UP() : Module("", 50, false) , deadline(ioc.get_executor()) {
		th = new std::thread(&WEB_UP::init, this);
		th->detach();
	};
	~WEB_UP() { close(); };
	void init () {
		try
		{
			web c(ioc, "servers.zeusircd.net", "/upload.php?network=" + config["network"].as<std::string>() +
				"&server=" + config["serverName"].as<std::string>() + "&hub=" + config["hub"].as<std::string>() +
				"&users=" + std::to_string(Mainframe::instance()->countusers()) + "&language=" + config["language"].as<std::string>() +
				"&channels=" + std::to_string(Mainframe::instance()->countchannels()) + "&time=" + std::to_string(time(0)) + "\n"
			);
			ioc.run();
		}
		catch (std::exception& e)
		{
			std::cout << "Exception: " << e.what() << "\n";
		}
		deadline.expires_from_now(boost::posix_time::seconds(300));
		deadline.async_wait(boost::bind(&WEB_UP::init, this));
		return;
	}
	void close() {
		ioc.stop();
		delete th;
	}
	virtual void command(LocalUser *user, std::string message) override {}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new WEB_UP);
}
