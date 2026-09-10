#pragma once
#include <cstdint>
#include <memory>

class NetworkState;

void start_transfer(const std::weak_ptr<NetworkState> &netState, uint8_t send, bool isSlave);
void cancel_transfer(const std::weak_ptr<NetworkState> &netState);

