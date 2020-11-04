//
// Created by user on 23.08.2020.
//

#ifndef IPCFLASHER_CONTROLLERELS27_H
#define IPCFLASHER_CONTROLLERELS27_H

#include <vector>
#include "../CanController.h"
extern "C" {
    #include "els27.h"
}

class ControllerEls27: public CanController {

    static const unsigned io_buff_max_len = 4096; //alloc 1kb buffer
    char io_buff[io_buff_max_len] = {};
    int tty_fd = 0;
    std::string m_port_name;

    inline static void ascii2hex(const char* in_str, uint8_t* out_hex);
    inline static void hex2ascii(const uint8_t* bin, unsigned int binsz, char* result);
    int serial_transaction(unsigned request_len);
    int serial_write(const char *request, unsigned request_len);
    void control_msg(const std::string &req);

public:

    explicit ControllerEls27(std::string port_name);
    ControllerEls27(ControllerEls27 const &) = delete;

    int init() override;
    int set_ecu_address(unsigned ecu_address) override;
    int set_protocol(CAN_PROTO protocol) override;

    int RAW_transaction(std::vector<uint8_t> &data) override;
};


#endif //IPCFLASHER_CONTROLLERELS27_H
