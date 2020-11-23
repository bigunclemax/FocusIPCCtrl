//
// Created by user on 14.10.2020.
//

#ifndef IPCFLASHER_DUMP_H
#define IPCFLASHER_DUMP_H

#include <mutex>
#include "controllers/CanController.h"
#include <vector>

void fakeFuel(CanController *controller, uint16_t fuel) {

    fuel = 0xf00 - (0x2a + fuel*235/50);

    std::vector<uint8_t> data = {0x00, 0x00,
                                 (uint8_t)(0x10u | ((fuel >> 8u) & 0xFu)),
                                 (uint8_t)(fuel & 0xFFu),
                                 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x320, data);
}

void fakeEngineRpmAndSpeed(CanController *controller, uint16_t rpm, uint16_t speed, bool speed_warning) {

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

    uint8_t acc = (g_acc_status) ? 0x67 : 0x00;
    if (!g_acc_standby)
        acc = 0x07;
    uint8_t door = (!g_drv_door) | (!g_psg_door << 1u) | (!g_rdrv_door << 2u) | (!g_rpsg_door << 3u) | (!g_boot << 4u) | (!g_hood << 5u);
    std::vector<uint8_t> data = { 0x77, 0x03, 0x07, door, 0xD9, acc, 0x03, 0x82 };
    controller->transaction(0x080, data);
}

void fakeEngineTemp(CanController *controller, uint8_t temp) {

    temp += 60; //C

    std::vector<uint8_t> data = { 0xe0, 0x00, 0x38, 0x40, 0x00, 0xe0, 0x69, temp};
    controller->transaction(0x360, data);
}

void fakeTurn(CanController *controller, bool left, bool right, uint16_t cruise_speed) {

    std::vector<uint8_t>  data = { 0x82, static_cast<unsigned char>(0x83u | (right << 3u) | (left << 2u)),
                                  0x00, 0x02, 0x80, static_cast<unsigned char>((cruise_speed * 20u/11) & 0xFFu), 0x00, 0x00 };
    controller->transaction(0x03A, data);
}

void accSimulateDistance(CanController* controller, bool accStatus, bool accStandby) {

    if (accStatus && !accStandby) {
        std::vector<uint8_t>  data = { 0x03, 0xd2, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00 };
        controller->transaction(0x020, data);
    }
}

void accSetDistance(CanController* controller, uint8_t accDistance, uint8_t accDistance2, bool accStatus, bool accStandby) {

    if (accStatus && accStandby) {
        accDistance = accDistance * 0x10;
        std::vector<uint8_t>  data = { 0x00, 0x9C, accDistance, 0x80, 0x11, 0xF4, 0xE8, 0x54 };
        controller->transaction(0x070, data);
    }
    else if (accStatus && !accStandby) {
        accDistance2 = 0xEF + (accDistance2 * 2);
        std::vector<uint8_t>  data = { 0x00, 0x9C, 0x22, 0x22, accDistance2, 0xF4, 0xE8, 0x54 };
        controller->transaction(0x070, data);
    }
}

void playAlarm(CanController* controller, bool alarm) {

    if (alarm) {
        std::vector<uint8_t>  data = { 0x00, 0xC0, 0x80, 0x21, 0xC0, 0x80, 0x00, 0x00 };
        controller->transaction(0x300, data);
    }
}

void changeDimming(CanController* controller, int dimming) {

    uint8_t dimLevel = (dimming) ? 0x05 : 0x06;
    std::vector<uint8_t>  data = { 0x98, 0x00, 0x1, 0x00, dimLevel, 0x00, 0x00, 0x00 };
    controller->transaction(0x290, data);
}

void fakeExternalTemp(CanController* controller, uint16_t temp) {

    temp = 0x0160 + (temp + 40) * 4;

    std::vector<uint8_t> data = { 0x00, 0x00, 0x00,
                                  static_cast<unsigned char>(((temp >> 8u) & 0xFu)),
                                  static_cast<unsigned char>((temp & 0xFFu)), 0x80, 0x00, 0x01 };
    controller->transaction(0x1a4, data);

    data = { 0x06, 0x00, 0x00, 0x00, 0x1e, 0x36, 0x9e, 0x4c };
    controller->transaction(0x2a0, data);
}

void dpfStatus(CanController* controller, bool full, bool regen) {

    uint8_t dpfStatus = (full) ? 0x88 : 0x66;
    if (!full && !regen)
        dpfStatus = 0x00;
    std::vector<uint8_t>  data = { 0x00, 0x00, 0x00, 0x00, 0x00, dpfStatus, 0x00, 0x00 };
    controller->transaction(0x083, data);
}

void resetDash(CanController *controller) {
    static bool t = false;
    std::vector<uint8_t>  data = { 0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x720, data);
}


#endif //IPCFLASHER_DUMP_H
