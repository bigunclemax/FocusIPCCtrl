//
// Created by user on 30.10.2020.
//

#include <iomanip>
#include <iostream>
#include <utility>
#include "QControllerEls27.h"

const uint32_t QControllerEls27::baud_arr[] = {
        19200,
        38400,
        57600,
        115200,
        230400,
        460800,
        500000,
        576000,
        921600,
        1000000,
        1152000,
        1500000,
        2000000,
        2500000,
        3000000,
        3500000,
        4000000 };

const int QControllerEls27::baud_arr_sz = sizeof(baud_arr) / sizeof(baud_arr[0]);

QControllerEls27::QControllerEls27(settings init_settings)
    : init_settings(std::move(init_settings))
{}

std::pair<int, std::string> QControllerEls27::serial_transaction(const std::string &req) {

    std::cerr << "W:" << req << std::endl << std::endl;

    write(req.c_str());

    uint8_t buffer[4096];
    unsigned response_max_len = sizeof(buffer);
    int idx = 0;
    do {
        if(!waitForReadyRead(1000)) {
            printf("waitForReadyRead timeout\n");
            break;
        }
        ssize_t rdlen = readLine(reinterpret_cast<char *>(&buffer[idx]), response_max_len - idx - 1);
        if (rdlen > 0) {
            idx += rdlen;
            if('>' == buffer[idx-1]  || '>' == buffer[idx-2]) {
                break;
            }
        } else if (rdlen < 0) {
            printf("Error from read. Reared %ld Error: %s", rdlen, strerror(errno));
            return {-1, ""};
        } else {  /* rdlen == 0 */
            printf("Nothing read. EOF?\n");
            return {-1, ""};
        }
        /* repeat read */
    } while (true);

    std::string s(buffer, buffer+idx);
    std::replace( s.begin(), s.end(), '\r', '-');
    std::cerr << "R:" << s << std::endl << std::endl;

    return {0, std::string(buffer, buffer+idx)};
}

void QControllerEls27::control_msg(const std::string &req) {
    serial_transaction(req + '\r');
}

void QControllerEls27::RAW_transaction(std::vector<uint8_t> &data) {
    auto tx_size = data.size() > CAN_frame_sz ? 8 : data.size();
    char io_buff[tx_size * 2 + 1];
    io_buff[tx_size * 2] = '\r';
    hex2ascii(data.data(), tx_size, io_buff);
    std::string req(io_buff, sizeof(io_buff));
    serial_transaction(req);
}

int QControllerEls27::set_ecu_address(unsigned int ecu_address) {
    std::stringstream ss;
    ss << "ATSH" << std::hex << std::setfill('0') << std::setw(3) <<  ecu_address;
    control_msg(ss.str());
    return 0;
}

int QControllerEls27::set_protocol(CanController::CAN_PROTO protocol) {
    if(CAN_MS == protocol)
        control_msg("STP53");   //ISO 15765, 11-bit Tx, 125kbps, DLC=8

    return 0;
}

int QControllerEls27::init() {

    setPortName(QString(init_settings.port_name.c_str()));
    if (!open(QIODevice::ReadWrite)) {
        throw std::runtime_error(QString(("failed to open port %1")).arg(portName()).toLatin1());
    }
    if(!setStopBits(QSerialPort::OneStop))
    {
        throw std::runtime_error(QString(("failed to set stop bits on port %1")).arg(portName()).toLatin1());
    }
    if(!setParity(QSerialPort::NoParity))
    {
        throw std::runtime_error(QString(("failed to set parity bits on port %1")).arg(portName()).toLatin1());
    }
    if(!setFlowControl(QSerialPort::NoFlowControl))
    {
        throw std::runtime_error(QString(("failed to set flow control on port %1")).arg(portName()).toLatin1());
    }

    clear();

    if(init_settings.baud) {
        if(test_baudrate(init_settings.baud)) {
            throw std::runtime_error("failed to connect adapter");
        }
    } else {
        if(!detect_baudrate()) {
            throw std::runtime_error("failed to connect adapter");
        }
    }

    if(init_settings.maximize) {
        maximize_baudrate();
    }

    std::cerr << "Els baud rate: " << baudRate() << std::endl;

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

int QControllerEls27::test_baudrate(uint32_t baud) {

    printf("Check baud: %d\n", baud);

    if(!setBaudRate(baud))
    {
        return -1;
    }

    /* clear */
    if(serial_transaction("?\r").first) {
        return -1;
    }

    auto r = serial_transaction("ATWS\r");
    if(r.first) {
        return -1;
    }

    if (r.second.find("ELM327 v1.3a") == std::string::npos) {
        return -1;
    }

    return 0;
}

int QControllerEls27::detect_baudrate() {
    for(unsigned int i : baud_arr) {
        if(!test_baudrate(i))
            return i;
    }
    return 0;
}

int QControllerEls27::set_baudrate(uint32_t baud) {

    auto curr_baud = baudRate();

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    int stbr_str_sz = snprintf(io_buff, io_buff_max_len, "STBR %d\r", baud);
    if(stbr_str_sz < 0) {
        return -1;
    }

    /* Host sends STBR */
    write(io_buff);


    waitForReadyRead();
    unsigned rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        return -1;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        return -1;
    }

    /* Host: switch to new baud rate */
    if(!setBaudRate(baud))
    {
        return -1;
    }

    waitForReadyRead();
    rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        goto cleanup;
    }

    /* Host: received a valid STI string? */
    if(strstr(io_buff, "STN1170 v3.3.1") == nullptr) {
        goto cleanup;
    }

    write("\r");

    waitForReadyRead();
    rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        goto cleanup;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        goto cleanup;
    }

    return 0;

cleanup:

    setBaudRate(curr_baud);

    return -1;
}

int QControllerEls27::maximize_baudrate() {

    /* set baudrate timeout in ms */
    if(serial_transaction("STBRT 1000\r").first) {
        return -1;
    }

    //TODO: find pos in sorted arr
    auto baud = baudRate();
    int i=0;
    while (baud != baud_arr[i]) {
        if(++i > baud_arr_sz) {
            return 0; //already maximized
        }
    }

    for(int j = i + 1; j < baud_arr_sz; ++j) {
        if(set_baudrate(baud_arr[j])) { //if ok, goes next
            continue;
        }
        baud = baud_arr[j];
    }

    return baud;
}