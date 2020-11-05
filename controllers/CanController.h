//
// Created by user on 05.10.2020.
//

#ifndef IPCFLASHER_CANCONTROLLER_H
#define IPCFLASHER_CANCONTROLLER_H

#include <string>
#include <functional>
#include <vector>

constexpr unsigned CAN_frame_sz = 8;

class CanController {
public:
    typedef enum {
        CAN_MS,
        CAN_HS
    } CAN_PROTO;

    virtual ~CanController() = default;
    virtual int init() = 0;
    virtual int set_ecu_address(unsigned ecu_address) =0;
    virtual int set_protocol(CAN_PROTO protocol) =0;

    virtual int RAW_transaction(std::vector<uint8_t> &data) =0;
};

#endif //IPCFLASHER_CANCONTROLLER_H