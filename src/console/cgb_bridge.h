#pragma once
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#ifdef BUILD_DESKTOP
#include "asio.hpp"
#endif


class CgbImplementation;


class TCPConnection : public std::enable_shared_from_this<TCPConnection>{
    public:
    #ifdef BUILD_DESKTOP
    TCPConnection(asio::io_context& io_context, CgbImplementation* cgb): socket_(io_context), cgb(cgb){
    
    }
    #else
    TCPConnection(CgbImplementation* cgb): cgb(cgb){
    
    }
    #endif
    void handle_write(const std::error_code& error, size_t bytes_transferred);
    void handle_read(const std::error_code& error, size_t bytes_transferred);
    #ifdef BUILD_DESKTOP
    asio::ip::tcp::socket socket_;
    #endif
    CgbImplementation* cgb;
    bool isSlave = false;
    bool cancelTransfer = false;
    std::array<uint8_t, 1> linkCableBufferIn = {0};
    std::array<uint8_t, 1> linkCableBufferOut = {0};
    typedef std::shared_ptr<TCPConnection> pointer;

    #ifdef BUILD_DESKTOP
    static pointer create(asio::io_context& io_context, CgbImplementation* cgb){
        return pointer(new TCPConnection(io_context, cgb));
    }
    #else
    static pointer create(CgbImplementation* cgb){
        return pointer(new TCPConnection(cgb));
    }
    #endif

    void writeByte(uint8_t data){
        #ifdef BUILD_DESKTOP
        linkCableBufferOut[0] = data;
        asio::async_write(socket_, asio::buffer(linkCableBufferOut),
        std::bind(&TCPConnection::handle_write, shared_from_this(),
          asio::placeholders::error,
          asio::placeholders::bytes_transferred));
        #endif

    }
    void readByte(){
        #ifdef BUILD_DESKTOP
        asio::async_read(socket_, asio::buffer(linkCableBufferIn), std::bind(&TCPConnection::handle_read, shared_from_this(),
           asio::placeholders::error, asio::placeholders::bytes_transferred));
        #endif
    }
};

struct NetworkState{
    #ifdef BUILD_DESKTOP
    std::optional<asio::ip::tcp::acceptor> acceptor = {};
    std::optional<TCPConnection::pointer> connection = {};
    #endif
    bool connectionReady = false;
    char portBuffer[6] = {'5','5','5','5','5','\0'};
    char hostBuffer[200] = {'l','o','c','a','l','h','o','s','t','\0'};
    char portBufferClient[6] = {'5','5','5','5','5','\0'};

    void handleAccept(TCPConnection::pointer con, const std::error_code& code);
    void renderMenuDesktop(CgbImplementation* cgb);
};

void start_transfer(const std::weak_ptr<NetworkState> &netState, uint8_t send, bool isSlave);
void cancel_transfer(const std::weak_ptr<NetworkState> &netState);
