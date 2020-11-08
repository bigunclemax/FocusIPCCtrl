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

    fuel = 0xf00 - (0x2a + fuel*235/50);

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

void fakeIgnitionDoors(CanController *controller, uint8_t g_drv_door, uint8_t g_psg_door, uint8_t g_rdrv_door, uint8_t g_rpsg_door, uint8_t g_hood,
                       uint8_t g_boot) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x080)) return;
    uint8_t door = (!g_drv_door) | (!g_psg_door << 1u) | (!g_rdrv_door << 2u) | (!g_rpsg_door << 3u) | (!g_hood << 4u) | (!g_boot << 5u);
    std::vector<uint8_t> data = { 0x77, 0x03, 0x07, door, 0xD9, 0x07, 0x03, 0x82 };
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
