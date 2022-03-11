//
// Created by user on 14.10.2020.
//

#ifndef IPCFLASHER_DUMP_H
#define IPCFLASHER_DUMP_H

#include <mutex>
#include "controllers/CanController.h"
#include <vector>

void package_320_FuelLevel(CanController *controller, uint16_t fuel) {

    fuel = 0xf00 - (0x2a + fuel*235/50);

    std::vector<uint8_t> data = {0x00, 0x00,
                                 (uint8_t)(0x10u | ((fuel >> 8u) & 0xFu)),
                                 (uint8_t)(fuel & 0xFFu),
                                 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x320, data);
}

void package_110_EngineRpmAndSpeed(CanController *controller, uint16_t rpm, uint16_t speed, bool speed_warning) {

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

//63-bit key battery low
//55-bit low beam

void package_080(CanController *controller, bool g_drv_door, bool g_psg_door, bool g_rdrv_door, bool g_rpsg_door,
                 bool g_hood, bool g_boot, bool g_head_lights, bool g_cruise_status, bool g_cruise_standby,
                 bool g_acc_on) {

    uint8_t acc = (g_cruise_status && !g_acc_on) ? 0xa7 : 0x07;
    if (g_cruise_status && g_cruise_standby) acc = 0x67;
    uint8_t door = (!g_drv_door) | (!g_psg_door << 1u) | (!g_rdrv_door << 2u) | (!g_rpsg_door << 3u) | (!g_boot << 4u) | (!g_hood << 5u);
    std::vector<uint8_t> data = { 0x77, static_cast<unsigned char>((g_head_lights << 7u) | 0x03), 0x07, door, 0xD9, acc, 0x03, 0x82 };
    controller->transaction(0x080, data);
}

void package_360_EngineTemp(CanController *controller, uint8_t temp) {

    temp += 60; //C

    std::vector<uint8_t> data = { 0xe0, 0x00, 0x38, 0x40, 0x00, 0xe0, 0x69, temp};
    controller->transaction(0x360, data);
}

/// 2 BYTE (turns)

/// 5 BYTE
//f8 - steering malfunction
//fe - steering loss
//80 - ok
//81 - speed + 133

/// 6 BYTE
//xx - speed from 0-133 or 134-268
void package_03A(CanController *controller, bool left, bool right, uint16_t cruise_speed) {
    //FIXME: cruise formula not accurate, i.e. it fails on 53 km
    std::vector<uint8_t>  data = { 0x82, static_cast<unsigned char>(0x83u | (right << 3u) | (left << 2u)),
                                  0x00, 0x02, static_cast<unsigned char>(0x80u|(cruise_speed>133)),
                                  (uint8_t)(cruise_speed * 0x200/268), 0x00, 0x00 };
    controller->transaction(0x03A, data);
}

void package_040(CanController *controller, bool airbagStatus, int averageSpeed, int millageTrip) {

    std::vector<uint8_t> data = { 0x66, 0x07, 0x40, 0xff, 0xbe, 0x57, 0x38, 0x00 };
    controller->transaction(0x040, data);
}

void package_020_accSimulateDistance(CanController* controller, bool accStatus, bool accStandby) {

    if (accStatus && !accStandby) {
        std::vector<uint8_t>  data = { 0x03, 0xd2, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00 };
        controller->transaction(0x020, data);
    }
}

void package_070_accSetDistance(CanController *controller, uint8_t accDistance, uint8_t accDistance2, bool accStatus,
                                bool accStandby) {

    std::vector<uint8_t>  data = { 0x00, 0x9C, 0x04, 0x80, 0x00, 0xF4, 0xE8, 0x54 };

    if (accStatus && accStandby) {
        accDistance = accDistance * 0x10;
        data[2] = accDistance;
        data[4] = 0x11;
    }
    else if (accStatus) {
        accDistance2 = 0xEF + (accDistance2 * 2);
        data[2] = 0x22;
        data[3] = 0x22;
        data[4] = accDistance2;
    }

    controller->transaction(0x070, data);
}

void package_300_playAlarm(CanController* controller, bool alarm) {

    std::vector<uint8_t>  data = { 0x00, 0x00, 0x80, 0x21, 0xC0, 0x00, 0x00, 0x00 };

    if (alarm) {
        data[1] = 0xC0;
        data[5] = 0x80;
    }

    controller->transaction(0x300, data);
}

void package_1A4_2A0_OutsideTemp(CanController* controller, uint16_t temp) {

    temp = 0x0160 + (temp + 40) * 4;

    std::vector<uint8_t> data = { 0x00, 0x00, 0x00,
                                  static_cast<unsigned char>(((temp >> 8u) & 0xFu)),
                                  static_cast<unsigned char>((temp & 0xFFu)), 0x80, 0x00, 0x01 };
    controller->transaction(0x1a4, data);

    data = { 0x06, 0x00, 0x00, 0x00, 0x1e, 0x36, 0x9e, 0x4c };
    controller->transaction(0x2a0, data);
}

void package_1A8(CanController* controller, bool highBeam, bool rearFog, uint8_t shift_advice, uint8_t averageFuel, uint8_t instantFuel) {

    //0040000100ff0000
    std::vector<uint8_t> data = { 0x2e, static_cast<unsigned char>(0xc0u | highBeam), static_cast<unsigned char>(0x2cu | rearFog << 6), 0x0f, 0x8e, instantFuel, 0xe1, 0xba };
    controller->transaction(0x1a8, data);
}

//limit data
//80-enabled
//40-standby
//00-off
void package_1B0_Lim(CanController* controller, bool limitStatus, bool limitStandby, uint8_t hillAssistStatus) {

    std::vector<uint8_t> data = { 0x01, 0x52, 0x27, 0x00,
                                  static_cast<unsigned char>(0x00u | ((limitStatus << 7u) >> limitStandby)),
                                  0xc0, 0x80, 0x00 };
    controller->transaction(0x1b0, data);
}

void package_1E0(CanController* controller, uint8_t immoStatus) {

     //TODO: last 4 bytes looks like growing uint32
    uint8_t dimm_mode =  0x80; // 0x00 - day, 0x80 - night
    std::vector<uint8_t> data = {0x42, dimm_mode, 0x1c, 0x00, 0x12, 0x03, 0x91, 0xe3 };
    controller->transaction(0x1e0, data);
}

void package_240(CanController *controller, bool brakeApplied) {

    std::vector<uint8_t> data = { 0x00, 0x02, 0x00, static_cast<unsigned char>(0x40u | (brakeApplied << 7u)), //0x80 - with alert
                                  0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x240, data);
}

void package_250(CanController *controller, bool oilStatus, bool engineStatus) {

    std::vector<uint8_t> data = { 0x20, 0xd5, 0x14, 0x0b, 0x08, 0x13, 0x16, 0x35 };
    controller->transaction(0x250, data);
}
//48 - water in fuel
//980001080000f1d0
//|      |
//|      |<----------------- lamp errors status
//|<----------------- ABS status
void package_290(CanController *controller, int dimming) {

    uint8_t dimLevel = (dimming) ? 0x05 : 0x06;
    std::vector<uint8_t>  data = { 0x98, 0x00, 0x01, 0x00, dimLevel, 0x00, 0x00, 0x00 };
    controller->transaction(0x290, data);
}

void package_083_dpfStatus(CanController* controller, bool full, bool regen) {

    uint8_t dpfStatus = (full) ? 0x88 : 0x66;
    if (!full && !regen)
        dpfStatus = 0x00;
    std::vector<uint8_t>  data = { 0x00, 0x00, 0x00, 0x00, 0x00, dpfStatus, 0x00, 0x00 };
    controller->transaction(0x083, data);
}

void package_508(CanController *controller, bool batteryStatus) {
    //byte[1] if - 0x12 day, if 0x00 - night
    std::vector<uint8_t> data = { static_cast<unsigned char>(0x10u | !batteryStatus), 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00 };
    controller->transaction(0x508, data);
}

void package_debug(CanController *controller, uint32_t ID, std::vector<uint8_t> &data) {

    controller->transaction(ID, data);
}

void resetDash(CanController *controller) {
    static bool t = false;
    std::vector<uint8_t>  data = { 0x02, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    controller->transaction(0x720, data);
}


#endif //IPCFLASHER_DUMP_H
