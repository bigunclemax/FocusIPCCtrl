//
// Created by user on 06.12.2020.
//

#ifndef FOCUSIPCCTRL_CONTROLLERELX_H
#define FOCUSIPCCTRL_CONTROLLERELX_H

#include "../CanController.h"
#include "stn1170-tools/stnlib/CanDevice.h"
#include <mutex>
#include <atomic>

using namespace stnlib;

struct sControllerSettings {
    std::string port_name;
    uint32_t    baud;
    bool        maximize;
};

class ControllerElx: public CanController {

public:

    explicit ControllerElx(sControllerSettings init_settings);
    ControllerElx(ControllerElx const &) = delete;

    int init() override { return 0; };
    int set_protocol(CAN_PROTO protocol) override;
    int transaction(unsigned ecu_address, std::vector<uint8_t> &data) override;
    void set_logger(CanLogger *logger) override;
    void remove_logger() override;
private:

    int send_data(std::vector<uint8_t> &data);
    int set_ecu_address(unsigned ecu_address);

    sControllerSettings     m_init_settings;

    CanDevice elxAdapter;
    std::mutex mutex;
    CanLogger* m_logger{};
    std::atomic<bool> is_logger_set = false;
    bool m_isElm327;

    inline static void hex2ascii(const uint8_t* bin, unsigned int binsz, char* result)
    {
        unsigned char     hex_str[]= "0123456789ABCDEF";

        for (auto i = 0; i < binsz; ++i)
        {
            result[i * 2 + 0] = (char)hex_str[(bin[i] >> 4u) & 0x0Fu];
            result[i * 2 + 1] = (char)hex_str[(bin[i]      ) & 0x0Fu];
        }
    };
};


#endif //FOCUSIPCCTRL_CONTROLLERELX_H
