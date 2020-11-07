#ifndef NET_SESSION_H
#define NET_SESSION_H

#include <mutex>
#include <map>
#include <deque>
#include <memory>
#include <functional>
#include <chrono>


#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <SharedCreator.hpp>


namespace Net
{
  class ClientHTTPSession;
  using ClientHTTPSessionPtr = std::shared_ptr<ClientHTTPSession>;

  class ClientHTTPSession: public SharedCreator<ClientHTTPSession>
    , public std::enable_shared_from_this<ClientHTTPSession>
  {
  public:
    using OpenHandler = std::function<void(const ClientHTTPSessionPtr&)>;
    using CloseHandler = std::function<void(const ClientHTTPSessionPtr&)>;
    using ReceiveHandler = std::function<void(const std::shared_ptr<std::string>&, const ClientHTTPSessionPtr&)>;

    void configure(const OpenHandler& openHandler
      , const CloseHandler& closeHandler, const ReceiveHandler& receiveHandler);

    void send(const std::shared_ptr<std::string>& payload);
    void drop();

    void open();
    void cancelAllPendingOperations();
    void close();

    boost::asio::ip::tcp::socket& socket();

  protected:
    ClientHTTPSession(boost::asio::io_context& ioService, boost::asio::ssl::context& context);
    virtual ~ClientHTTPSession();

  private:
    ClientHTTPSession(const ClientHTTPSession&) = delete;
    ClientHTTPSession(ClientHTTPSession&&) = delete;
    ClientHTTPSession& operator=(const ClientHTTPSession&) = delete;
    ClientHTTPSession& operator=(ClientHTTPSession&&) = delete;

  private:
    void configureSocket();

    void readNext();
    void writeNext();

    void asyncHandshake();
    void asyncRead(const std::shared_ptr<std::string>& payload);
    void asyncWrite(const std::shared_ptr<std::string>& payload);

    void handleHandshake(const boost::system::error_code& error);
    void handleRead(const std::shared_ptr<std::string>& payload, const boost::system::error_code& error, std::size_t bytes_transferred);
    void handleWrite(const std::shared_ptr<std::string>& payload, const boost::system::error_code& error);

    void handleMessage(const std::shared_ptr<std::string>& payload);

  private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> socketStrand_;
    boost::asio::strand<boost::asio::io_context::executor_type> readStrand_;

    OpenHandler openHandler_;
    CloseHandler closeHandler_;
    ReceiveHandler receiveHandler_;

    std::deque<std::shared_ptr<std::string>> writeQueue_;
    std::recursive_mutex queueMutex_;

    bool isConnected_;
  };
}

#endif // !NET_SESSION_H
