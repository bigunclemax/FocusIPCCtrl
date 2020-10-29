//
// Created by user on 23.08.2020.
//
#include "ControllerEls27.h"

#include <iostream>
#include <utility>
#include <vector>
#include <iomanip>

constexpr unsigned ASCII_MSG_SIZE = CAN_frame_sz*2;


ControllerEls27::ControllerEls27(std::string port_name): m_port_name(std::move(port_name)) {
    tty_fd = els27_open_port(("/dev/"+m_port_name).c_str());
}

int ControllerEls27::init() {

//    if(els27_check_baudrate(tty_fd, B2000000)) {
//        auto baud = els27_maximize_speed(tty_fd);
//        std::cout << "Maximized baud: " << baud2int(baud) << std::endl;
//        if(!baud) return -1;
//    }

//    control_msg("ATZ\r"); //reset
    control_msg("\r");
    control_msg("ATE0"); //echo off
    control_msg("ATL0"); //linefeeds off
    control_msg("ATS0"); //spaces off

    control_msg("STPO");  //ATBI Open current protocol.
    control_msg("ATAL");  //allow long messages
    control_msg("ATAT0"); //disable adaptive timing
    control_msg("ATCAF0");//CAN auto formatting off

    control_msg("ATST01");  //Set timeout to hh(13) x 4 ms
    control_msg("ATR0");    //Responses on

    return 0;
}

int ControllerEls27::set_ecu_address(unsigned int ecu_address) {

    std::stringstream ss;
    ss << "ATSH" << std::hex << std::setfill('0') << std::setw(3) <<  ecu_address;
    control_msg(ss.str());

    return 0;
}

int ControllerEls27::set_protocol(CAN_PROTO protocol) {
    if(CAN_MS == protocol)
        control_msg("STP53");   //ISO 15765, 11-bit Tx, 125kbps, DLC=8

    return 0;
}

int ControllerEls27::serial_write(const char *request, unsigned request_len) {
    return els27_write(tty_fd, request, request_len);
}

int ControllerEls27::serial_transaction(unsigned request_len) {
    unsigned response_len = io_buff_max_len;
    io_buff[request_len] = '\r';
    if(els27_transaction(tty_fd, io_buff, request_len+1, io_buff, &response_len)) {
        return -1;
    } else {
        return (int)response_len;
    }
}

void ControllerEls27::control_msg(const std::string &req) {

    req.copy(io_buff, req.size());
    serial_transaction(req.size());
}


void ControllerEls27::RAW_transaction(std::vector<uint8_t> &data) {
    auto tx_size = data.size() > CAN_frame_sz ? 8 : data.size();
    hex2ascii(data.data(), tx_size, io_buff);
    serial_transaction(tx_size*2);
}

constexpr int asciiHexToInt(uint8_t hexchar) {
    return (hexchar >= 'A') ? (hexchar - 'A' + 10) : (hexchar - '0');
}

void ControllerEls27::ascii2hex(const char* in_str, uint8_t* out_hex) {
    out_hex[0] = asciiHexToInt(in_str[0]) << 4 | asciiHexToInt(in_str[1]);
    out_hex[1] = asciiHexToInt(in_str[2]) << 4 | asciiHexToInt(in_str[3]);
    out_hex[2] = asciiHexToInt(in_str[4]) << 4 | asciiHexToInt(in_str[5]);
    out_hex[3] = asciiHexToInt(in_str[6]) << 4 | asciiHexToInt(in_str[7]);
    out_hex[4] = asciiHexToInt(in_str[8]) << 4 | asciiHexToInt(in_str[9]);
    out_hex[5] = asciiHexToInt(in_str[10]) << 4 | asciiHexToInt(in_str[11]);
    out_hex[6] = asciiHexToInt(in_str[12]) << 4 | asciiHexToInt(in_str[13]);
    out_hex[7] = asciiHexToInt(in_str[14]) << 4 | asciiHexToInt(in_str[15]);
}



void ControllerEls27::hex2ascii(const uint8_t* bin, unsigned int binsz, char* result)
{
    unsigned char     hex_str[]= "0123456789ABCDEF";

    for (auto i = 0; i < binsz; ++i)
    {
        result[i * 2 + 0] = (char)hex_str[(bin[i] >> 4u) & 0x0Fu];
        result[i * 2 + 1] = (char)hex_str[(bin[i]      ) & 0x0Fu];
    }
}