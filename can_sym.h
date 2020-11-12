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

void fakeEngineRpmAndSpeed(CanController *controller, uint16_t rpm, uint16_t speed, bool speed_warning) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x110)) return;
    uint8_t warning = (speed_warning) ? 0xdd : 0xc0;
    rpm /= 2;
    speed = speed * 10000 / 105; //km\h

    std::vector<uint8_t> data = { warning, 0xcf, 0x00, 0x00,
                                  (uint8_t)((rpm >> 8u)),
                                  (uint8_t)(rpm & 0xFFu),
                                  (uint8_t)((speed >> 8u)),
                                  (uint8_t)(speed & 0xFFu)};
    controller->RAW_transaction(data);
}

void fakeIgnitionMiscellaneous(CanController *controller, uint8_t g_drv_door, uint8_t g_psg_door, uint8_t g_rdrv_door, uint8_t g_rpsg_door, uint8_t g_hood,
                       uint8_t g_boot, uint8_t g_acc_status) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if(controller->set_ecu_address(0x080)) return;
    uint8_t acc = (g_acc_status) ? 0x67 : 0x00;
    uint8_t door = (!g_drv_door) | (!g_psg_door << 1u) | (!g_rdrv_door << 2u) | (!g_rpsg_door << 3u) | (!g_boot << 4u) | (!g_hood << 5u);
    std::vector<uint8_t> data = { 0x77, 0x03, 0x07, door, 0xD9, acc, 0x03, 0x82 };
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

void accDistance(CanController* controller, uint8_t accDistance, bool accStatus) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (accStatus) {
        accDistance = accDistance * 0x10;
        if (controller->set_ecu_address(0x070)) return;
        std::vector<uint8_t>  data = { 0x00, 0x9C, accDistance, 0x80, 0x11, 0xF4, 0xE8, 0x54 };
        controller->RAW_transaction(data);
    }
}

void resetDash(CanController *controller) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    static bool t = false;
    if(controller->set_ecu_address(0x720)) return;
    std::vector<uint8_t>  data = {0x02,0x11,0x01,0x00,0x00,0x00,0x00,0x00};
    controller->RAW_transaction(data);
}


#endif //IPCFLASHER_DUMP_H
