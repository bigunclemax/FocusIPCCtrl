//
// Created by user on 14.10.2020.
//

#ifndef IPCFLASHER_DUMP_H
#define IPCFLASHER_DUMP_H

#include <mutex>
#include "controllers/CanController.h"
#include <vector>

std::mutex m_mutex;

void fakeFuel(CanController *controller, uint16_t fuel) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x320)) return;

    //TODO: doesn't working correctly
    fuel = 0xfff - (fuel * 0x2ff / 100); //percents from 0 to 100

    std::vector<uint8_t> data = {0x00, 0x00,
                                 (uint8_t)(0x10u | ((fuel >> 8u) & 0xFu)),
                                 (uint8_t)(fuel & 0xFFu),
                                 0x00, 0x00, 0x00, 0x00 };
    controller->RAW_transaction(data);
}

void fakeEngineRpmAndSpeed(CanController *controller, uint16_t rpm, uint16_t speed) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x110)) return;

    rpm /= 2;
    speed = speed * 10000 / 105; //km\h

    std::vector<uint8_t> data = { 0xc0, 0x03, 0x0a, 0x01,
                                  (uint8_t)((rpm >> 8u)),
                                  (uint8_t)(rpm & 0xFFu),
                                  (uint8_t)((speed >> 8u)),
                                  (uint8_t)(speed & 0xFFu)};
    controller->RAW_transaction(data);
}

void fakeIgnition(CanController *controller) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x080)) return;

    std::vector<uint8_t> data = { 0x74, 0x63, 0xC7, 0x80, 0x10, 0x7A };
    controller->RAW_transaction(data);
}

void fakeEngineTemp(CanController *controller, uint8_t temp) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x360)) return;

    temp += 60; //C

    std::vector<uint8_t> data = { 0xe0, 0x00, 0x38, 0x40, 0x00, 0xe0, 0x69, temp};
    controller->RAW_transaction(data);
}

void fakeTurn(CanController *controller, bool left, bool right) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x03A)) return;
    std::vector<uint8_t>  data = { 0x82,
                                   static_cast<unsigned char>(0x83u | (right << 3u) | (left << 2u)),
                                   0x00, 0x02, 0x80, 0x00, 0x00, 0x00};
    controller->RAW_transaction(data);
}

void resetDash(CanController *controller) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    static bool t = false;
    if(controller->set_ecu_address(0x720)) return;
    std::vector<uint8_t>  data = {0x02,0x11,0x01,0x00,0x00,0x00,0x00,0x00};
    controller->RAW_transaction(data);
}


#endif //IPCFLASHER_DUMP_H
