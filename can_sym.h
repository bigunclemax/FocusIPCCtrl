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

    fuel = 0xf00 - (0x2a + fuel*235/50);

    std::vector<uint8_t> data = {0x00, 0x00,
                                 (uint8_t)(0x10u | ((fuel >> 8u) & 0xFu)),
                                 (uint8_t)(fuel & 0xFFu),
                                 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x320, data);
}

void fakeEngineRpmAndSpeed(CanController *controller, uint16_t rpm, uint16_t speed, bool speed_warning) {

    const std::lock_guard<std::mutex> lock(m_mutex);

    uint8_t warning = (speed_warning) ? 0xdd : 0xc0;
    rpm /= 2;
    speed = speed * 10000 / 105; //km\h

    std::vector<uint8_t> data = { warning, 0xcf, 0x00, 0x00,
                                  (uint8_t)((rpm >> 8u)),
                                  (uint8_t)(rpm & 0xFFu),
                                  (uint8_t)((speed >> 8u)),
                                  (uint8_t)(speed & 0xFFu)};
    controller->transaction(0x110, data);
}

void fakeIgnitionMiscellaneous(CanController *controller, uint8_t g_drv_door, uint8_t g_psg_door, uint8_t g_rdrv_door, uint8_t g_rpsg_door, uint8_t g_hood,
                       uint8_t g_boot, uint8_t g_acc_status, uint8_t g_acc_standby) {

    const std::lock_guard<std::mutex> lock(m_mutex);

    uint8_t acc = (g_acc_status) ? 0x67 : 0x00;
    if (!g_acc_standby)
        acc = 0x07;
    uint8_t door = (!g_drv_door) | (!g_psg_door << 1u) | (!g_rdrv_door << 2u) | (!g_rpsg_door << 3u) | (!g_boot << 4u) | (!g_hood << 5u);
    std::vector<uint8_t> data = { 0x77, 0x03, 0x07, door, 0xD9, acc, 0x03, 0x82 };
    controller->transaction(0x080, data);
}

void fakeEngineTemp(CanController *controller, uint8_t temp) {

    const std::lock_guard<std::mutex> lock(m_mutex);

    temp += 60; //C

    std::vector<uint8_t> data = { 0xe0, 0x00, 0x38, 0x40, 0x00, 0xe0, 0x69, temp};
    controller->transaction(0x360, data);
}

void fakeTurn(CanController *controller, bool left, bool right, uint16_t cruise_speed) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<uint8_t>  data = { 0x82, static_cast<unsigned char>(0x83u | (right << 3u) | (left << 2u)),
                                  0x00, 0x02, 0x80, static_cast<unsigned char>(cruise_speed & 0xFFu), 0x00, 0x00 };
    controller->transaction(0x03A, data);
}

void accSetDistance(CanController* controller, uint8_t accDistance, uint8_t accDistance2, bool accStatus, bool acc_standby) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (accStatus && acc_standby) {
        accDistance = accDistance * 0x10;
        std::vector<uint8_t>  data = { 0x00, 0x9C, accDistance, 0x80, 0x11, 0xF4, 0xE8, 0x54 };
        controller->transaction(0x070, data);
    } else if (accStatus && !acc_standby) {
        accDistance2 = accDistance2 * 0xA2;
        std::vector<uint8_t>  data = { 0x00, 0x9C, 0x22, 0x22, accDistance2, 0xF4, 0xE8, 0x54 };
        controller->transaction(0x070, data);
    }
}

void accSimulateDistance(CanController* controller, bool accStatus, bool acc_standby) {

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (accStatus && !acc_standby) {
        std::vector<uint8_t>  data = { 0x03, 0xd2, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00 };
        controller->transaction(0x020, data);
    }
}

void resetDash(CanController *controller) {
    const std::lock_guard<std::mutex> lock(m_mutex);
    static bool t = false;
    std::vector<uint8_t>  data = { 0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x720, data);
}


#endif //IPCFLASHER_DUMP_H
