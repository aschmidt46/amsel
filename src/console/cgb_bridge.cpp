#include "cgb_bridge.h"
#include "framework/global.h"
#include <memory>
#ifdef BUILD_DESKTOP
  #include <imgui.h>
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
    errorText = "Listening...";
    allowDisconnect = true;
    if(connectionReady)
      errorText = "connected.";
  }
  else{
    errorText = "idle.";
  }
  ImGui::Begin("Link Cable");
    ImGui::SeparatorText("Host");
    ImGui::InputText("Port", portBuffer, 6);
    ImGui::SameLine();
    if(ImGui::Button("Host")){
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
          {"Link Cable Error"},
          "Invalid Port Number"
        });
      }
      catch(std::exception &e){
        acceptor = {};
        messageQueue.enqueue(MessageStruct{
          MessageType::MT_ERROR,
          {"Link Cable Error"},
          e.what()
        });
      }
    }
    ImGui::Text("%s", errorText.c_str());
    if(allowDisconnect){
      ImGui::SameLine();
      if(ImGui::Button("Disconnect##server")){
        acceptor->close();
        acceptor = {};
        connection.value()->socket_.close();
        connection = {};
        connectionReady = false;
      }
    }
    ImGui::SeparatorText("Connect");
    ImGui::InputText("Host##2", hostBuffer, 200);
    ImGui::InputText("Port##2", portBufferClient, 6);
    if(ImGui::Button("Connect")){
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
          {"Link Cable Error"},
          e.what()
        });
      }
    }
    if(connectionReady){
      ImGui::Text("Connected");
    }
    else if(connection.has_value()){
      ImGui::Text("Connecting...");
    }
    else{
      ImGui::Text("idle.");
    }
    if(ImGui::Button("Disconnect##client")){
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



