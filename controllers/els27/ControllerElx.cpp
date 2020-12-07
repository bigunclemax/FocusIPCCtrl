//
// Created by user on 06.12.2020.
//

#include <iostream>
#include <iomanip>
#include <utility>
#include "ControllerElx.h"

ControllerElx::ControllerElx(sControllerSettings init_settings)
        : m_init_settings(std::move(init_settings))
        , comPort(SerialPort(m_init_settings.port_name, m_init_settings.baud, 1))
{

    auto sti_resp = comPort.serial_transaction("STI\r");
    if (sti_resp.first || sti_resp.second.find("STN") == std::string::npos) {
        m_isElm327 = true;
    }

    if(m_init_settings.maximize) {
        comPort.maximize_baudrate();
    }

    comPort.serial_transaction("ATE1\r");   //echo on
    comPort.serial_transaction("ATL0\r");   //linefeeds off
    comPort.serial_transaction("ATS0\r");   //spaces off
    comPort.serial_transaction("STPO\r");   //ATBI Open current protocol.
    comPort.serial_transaction("ATAL\r");   //allow long messages
    comPort.serial_transaction("ATAT0\r");  //disable adaptive timing
    comPort.serial_transaction("ATCAF0\r"); //CAN auto formatting off
    comPort.serial_transaction("ATST01\r"); //Set timeout to hh(13) x 4 ms
    comPort.serial_transaction("ATR0\r");   //Responses on
}

void ControllerElx::set_logger(CanLogger *logger) {
    m_logger = logger;
    is_logger_set = true;
}

void ControllerElx::remove_logger() {
    is_logger_set = false;
    m_logger = nullptr;
}

int ControllerElx::set_protocol(CanController::CAN_PROTO protocol) {

    const std::lock_guard<std::mutex> lock(mutex);
    if(CAN_MS == protocol) {
        if(m_isElm327) {
            comPort.serial_transaction("ATSPB\r");   // User1 CAN (11* bit ID, 125* kbaud
        } else {
            comPort.serial_transaction("STP53\r");   //ISO 15765, 11-bit Tx, 125kbps, DLC=8
        }
    }

    return 0;
}

int ControllerElx::transaction(unsigned int ecu_address, vector<uint8_t> &data) {

    const std::lock_guard<std::mutex> lock(mutex); //TODO: remove me if you want mess :D
    if(set_ecu_address(ecu_address)) {
        std::cerr << "Set ECU address 0x" << std::hex << ecu_address << " error" << std::endl;
        return -1;
    }

    if(send_data(data)) {
        std::cerr << "Send data to ECU 0x" << std::hex << ecu_address << " error" << std::endl;
        return -1;
    }

    if(is_logger_set) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(3) << ecu_address << ": " << std::setw(2);
        for(auto d : data) {ss << std::setfill('0') << std::setw(2) << (int)d;}
        m_logger->write(ss.str().c_str());
    }

    return 0;
}

int ControllerElx::send_data(std::vector<uint8_t> &data) {
    auto tx_size = data.size() > CAN_frame_sz ? 8 : data.size();
    std::string io_buff;
    io_buff.resize(tx_size * 2 + 1);
    io_buff[tx_size * 2] = '\r';
    hex2ascii(data.data(), tx_size, io_buff.data());
    return comPort.serial_transaction(io_buff).first;
}

int ControllerElx::set_ecu_address(unsigned int ecu_address) {
    std::stringstream ss;
    ss << "ATSH" << std::hex << std::setfill('0') << std::setw(3) <<  ecu_address << '\r';
    return comPort.serial_transaction(ss.str()).first;
}