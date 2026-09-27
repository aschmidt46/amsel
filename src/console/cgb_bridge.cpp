#include "cgb_bridge.h"
#include "framework/global.h"
#include <memory>
#ifdef BUILD_DESKTOP
  #include <imgui.h>
  #include "framework/global.h"
  #include "framework/locale.h"
#endif
#include "cgb_implementation.h"

#ifdef BUILD_DESKTOP
using asio::ip::tcp;
#endif

void TCPConnection::handle_write(const std::error_code& error, size_t bytes_transferred){
  #ifdef BUILD_DESKTOP
    if(bytes_transferred>0){
      // std::cout << "sent: " << int(linkCableBufferOut[0]) << "\n";
      cgb->transferSent();
    }
    else{ // Fehler
      if(isSlave){ // Slave schreibt nur, wenn extern geclockt, Datenverlust nicht erlaubt
        if(!cancelTransfer)
          writeByte(linkCableBufferOut[0]);
      }
      else{
        // std::cout << error.message() << std::endl;
        cgb->transferSent();
      }
    }
    #endif
}

void TCPConnection::handle_read(const std::error_code& error, size_t bytes_transferred){
  #ifdef BUILD_DESKTOP
  if(bytes_transferred==0){
      // std::cout << "received: " << int(linkCableBufferIn[0]) << "\n";
      linkCableBufferIn[0] = 0xFF;
  }
  if(isSlave){
    if(bytes_transferred > 0){ // Transfer erfolgreich = externer Clock -> schreiben, nach schreiben receive
      this->writeByte(linkCableBufferOut[0]);
      cgb->transferReceive(linkCableBufferIn[0]);
    }
    else{ // Falls Datenübertragung nicht erfolgreich, unbegrenzt oft wiederholen, bis es klappt (als Slave)
      if(!cancelTransfer)
        this->readByte();
      else{
        cancelTransfer = false;
      }
    }
  }
  else{
    // if(bytes_transferred==0)
      // std::cout << error.message() << std::endl;
    cgb->transferReceive(linkCableBufferIn[0]);
  }
  #endif
}

void NetworkState::renderMenuDesktop(CgbImplementation* cgb){
  #ifdef BUILD_DESKTOP
  std::string errorText = "";
  bool allowDisconnect = false;
  if(acceptor.has_value() && acceptor->is_open()){
    errorText = locale.getTranslation(LinkCableListening);
    allowDisconnect = true;
    if(connectionReady)
      errorText = locale.getTranslation(LinkCableConnected);
  }
  else{
    errorText = locale.getTranslation(LinkCableIdle);
  }
  ImGui::Begin(locale.getTranslation(LinkCable).c_str());
    ImGui::SeparatorText(locale.getTranslation(LinkCableHostSection).c_str());
    ImGui::InputText(locale.getTranslation(LinkCablePort).c_str(), portBuffer, 6);
    ImGui::SameLine();
    if(ImGui::Button(locale.getTranslation(LinkCableHostButon).c_str())){
      try{
        int port = std::stoi(portBuffer);
        if(port > std::numeric_limits<uint16_t>::max())
          throw std::invalid_argument("");
        acceptor = tcp::acceptor(io_context, tcp::endpoint(tcp::v6(), port));
        connection = TCPConnection::create(io_context, cgb);
        acceptor.value().async_accept(connection.value()->socket_,
          std::bind(&NetworkState::handleAccept, this, connection.value(), asio::placeholders::error));
      }
      catch(std::invalid_argument &e){
        acceptor = {};
        messageQueue.enqueue(MessageStruct{
          MessageType::MT_ERROR,
          {locale.getTranslation(LinkCableError)},
          locale.getTranslation(LinkCableInvalidPortNumber)
        });
      }
      catch(std::exception &e){
        acceptor = {};
        messageQueue.enqueue(MessageStruct{
          MessageType::MT_ERROR,
          {locale.getTranslation(LinkCableError)},
          e.what()
        });
      }
    }
    ImGui::Text("%s", errorText.c_str());
    if(allowDisconnect){
      ImGui::SameLine();
      std::string disconnectServer = locale.getTranslation(LinkCableDisconnectButton) + "##server";
      if(ImGui::Button(disconnectServer.c_str())){
        acceptor->close();
        acceptor = {};
        connection.value()->socket_.close();
        connection = {};
        connectionReady = false;
      }
    }
    ImGui::SeparatorText(locale.getTranslation(LinkCableConnectClient).c_str());
    std::string hostClient = locale.getTranslation(LinkCableConnectHostAddress) + "##2";
    std::string clientPort = locale.getTranslation(LinkCablePort) + "##2";
    ImGui::InputText(hostClient.c_str(), hostBuffer, 200);
    ImGui::InputText(clientPort.c_str(), portBufferClient, 6);
    if(ImGui::Button(locale.getTranslation(LinkCableConnectClientButton).c_str())){
      try{
        asio::ip::tcp::resolver resolver(io_context);
        connection = TCPConnection::create(io_context, cgb);
        auto endpoints = resolver.resolve(hostBuffer, portBufferClient);
        asio::async_connect(connection.value()->socket_, endpoints, std::bind([&](){
          connectionReady = true;
        }));
      }
      catch(std::exception &e){
        acceptor = {};
        connection = {};
        messageQueue.enqueue(MessageStruct{
          MessageType::MT_ERROR,
          {locale.getTranslation(LinkCableError)},
          e.what()
        });
      }
    }
    if(connectionReady){
      ImGui::Text(locale.getTranslation(LinkCableConnected).c_str());
    }
    else if(connection.has_value()){
      ImGui::Text(locale.getTranslation(LinkCableConnecting).c_str());
    }
    else{
      ImGui::Text(locale.getTranslation(LinkCableIdle).c_str());
    }
    std::string disconnectClientButton = locale.getTranslation(LinkCableDisconnectButton) + "##client";
    if(ImGui::Button(disconnectClientButton.c_str())){
        connection = {};
        connectionReady = false;
      }

  ImGui::End();
  #endif
}

void NetworkState::handleAccept(TCPConnection::pointer con, const std::error_code& code){
  #ifdef BUILD_DESKTOP
  if(!code){
    connectionReady = true;
    // clockLinkCable();
  }
  #endif
}

void start_transfer(const std::weak_ptr<NetworkState> &netState, uint8_t send, bool isSlave){
  #ifdef BUILD_DESKTOP
  if(netState.lock()->connection.has_value()){
      const auto con = netState.lock()->connection.value();
      con->isSlave = isSlave;
      con->cancelTransfer = false;
      con->readByte();
      con->linkCableBufferOut[0] = send;
      if(!isSlave){
        con->writeByte(send);
        // Master sendet und wartet auf Antwort
        // Slave wartet auf Clock + Daten und sendet?
      }
  }
  #endif
}

void cancel_transfer(const std::weak_ptr<NetworkState> &netState){
  #ifdef BUILD_DESKTOP
  if(netState.lock()->connection.has_value()){
    const auto con = netState.lock()->connection.value();
    con->cancelTransfer = true;
  }
  #endif
}



