#include "network.h"
#include "config.h"
#include <cstring>

Network::Network() : status(Status::DISCONNECTED), tcp_client(nullptr), wifi_connected(false) {}

bool Network::init() {
    if (cyw43_arch_init()) {
        status = Status::ERROR;
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    return true;
}

bool Network::connect_wifi() {
    if (wifi_connected) return true;
    
    status = Status::CONNECTING;
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, 
                                         CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        status = Status::ERROR;
        return false;
    }
    
    wifi_connected = true;
    status = Status::CONNECTED;
    return true;
}

bool Network::connect_tcp() {
    if (tcp_client) {
        tcp_close(tcp_client);
        tcp_client = nullptr;
    }
    
    tcp_client = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!tcp_client) return false;
    
    ip_addr_t server_addr;
    ipaddr_aton(SERVER_IP, &server_addr);
    
    tcp_arg(tcp_client, this);
    tcp_sent(tcp_client, tcp_sent_cb);
    tcp_err(tcp_client, tcp_error_cb);
    
    err_t err = tcp_connect(tcp_client, &server_addr, SERVER_PORT, tcp_connected_cb);
    return err == ERR_OK;
}

bool Network::send_data(const uint8_t* data, size_t len) {
    if (!tcp_client || status != Status::CONNECTED) return false;
    
    err_t err = tcp_write(tcp_client, data, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        status = Status::ERROR;
        return false;
    }
    
    err = tcp_output(tcp_client);
    return err == ERR_OK;
}

err_t Network::tcp_connected_cb(void* arg, struct tcp_pcb* tpcb, err_t err) {
    Network* network = static_cast<Network*>(arg);
    if (err != ERR_OK) {
        network->status = Status::ERROR;
        return err;
    }
    network->status = Status::CONNECTED;
    return ERR_OK;
}

void Network::tcp_error_cb(void* arg, err_t err) {
    Network* network = static_cast<Network*>(arg);
    network->tcp_client = nullptr;
    network->status = Status::ERROR;
}

err_t Network::tcp_sent_cb(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    return ERR_OK;
}