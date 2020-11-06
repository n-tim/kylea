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

#include <RandomNumberGenerator.hpp>
#include <SharedCreator.hpp>


#include "MessagePtr.hpp"
#include "MetaMessagePtr.hpp"

#include "SessionInterface.hpp"
#include "SessionPtr.hpp"

namespace Net
{
  class Session: public SessionInterface, public SharedCreator<Session>
    , public std::enable_shared_from_this<Session>
  {
    enum { PingTimeOut = 333, MaxTimeOut = PingTimeOut * 3 };
    enum { MaxMessagesToRead = 32};

  public:
    using OpenHandler = std::function<void(const SessionPtr&)>;
    using CloseHandler = std::function<void(const SessionPtr&)>;

    void configure(const OpenHandler& openHandler
      , const CloseHandler& closeHandler, const ReceiveHandler& receiveHandler);

    virtual void send(const BufferPtr& payload) override;
    virtual void drop() override;

    void open(const boost::asio::ssl::stream_base::handshake_type& handshakeType);
    void cancelAllPendingOperations();
    void close();

    boost::asio::ip::tcp::socket& socket();

  protected:
    Session(boost::asio::io_context& ioService, boost::asio::ssl::context& context);
    virtual ~Session();

  private:
    Session(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) = delete;

  private:
    void configureSocket();

    void ping();
    void pong();

    void send(const MessagePtr& message);

    void readNext();
    void writeNext();

    void asyncHandshake(const boost::asio::ssl::stream_base::handshake_type& handshakeType);
    void asyncReadHeader(const MetaMessagePtr& Message);
    void asyncReadBody(const MetaMessagePtr& Message);
    void asyncWrite(const MessagePtr& Message);

    void handleHandshake(const boost::system::error_code& error);
    void handleReadHeader(const MetaMessagePtr& Message, const boost::system::error_code& error);
    void handleReadBody(const MetaMessagePtr& Message, const boost::system::error_code& error);
    void handleWrite(const MessagePtr& Message, const boost::system::error_code& error);

    void onMessageReceived(const MetaMessagePtr& Message);
    void handleMessage(const MetaMessagePtr& message);

    void restartReadTimer();
    void cancelReadTimer();
    void handleReadTimeOut(const boost::system::error_code& error);

  private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> socketStrand_;
    boost::asio::strand<boost::asio::io_context::executor_type> readStrand_;

    boost::asio::deadline_timer readTimer_;
    boost::asio::strand<boost::asio::io_context::executor_type> readTimerStrand_;

    RandomNumberGenerator<float> randomNumberGenerator_;

    std::chrono::system_clock::time_point lastTimeRead_;

    OpenHandler openHandler_;
    CloseHandler closeHandler_;
    ReceiveHandler receiveHandler_;

    std::deque<MessagePtr> writeQueue_;
    std::recursive_mutex queueMutex_;

    bool isConnected_;
  };
}

#endif // !NET_SESSION_H
