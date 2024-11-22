#ifndef PICO_NETWORK_H
#define PICO_NETWORK_H


#include <functional>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

class Network {
public:
    enum class Status {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

    Network();
    bool init();
    bool connect_wifi();
    bool connect_tcp();
    bool send_data(const uint8_t* data, size_t len);
    Status get_status() const { return status; }
    
    // TCP callbacks
    static err_t tcp_connected_cb(void* arg, struct tcp_pcb* tpcb, err_t err);
    static void tcp_error_cb(void* arg, err_t err);
    static err_t tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, u16_t len);

private:
    Status status;
    struct tcp_pcb* tcp_client;
    bool wifi_connected;
};

#endif // PICO_NETWORK_H