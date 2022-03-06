//
// Created by user on 05.10.2020.
//

#ifndef CANCONTROLLER_H
#define CANCONTROLLER_H

#include <string>
#include <functional>
#include <vector>

constexpr unsigned CAN_frame_sz = 8;

typedef std::pair<uint32_t, std::vector<uint8_t>> can_packet;

class CanLogger {
public:
    virtual void write(const char *msg) = 0;
};

class CanController {
public:
    typedef enum {
        CAN_MS,
        CAN_HS
    } CAN_PROTO;

    /**
     *  CAN bus adapter interface
     */
    virtual int set_protocol(CAN_PROTO protocol) =0;
    virtual int transaction(unsigned ecu_address, std::vector<uint8_t> &data) =0;
    virtual void set_logger(CanLogger *logger) {};
    virtual void remove_logger() {};

};

#endif //CANCONTROLLER_H
