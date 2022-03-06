//
// Created by user on 17.12.2021.
//

#ifndef FOCUSIPCCTRL_CONTROLLERVIRTUAL_H
#define FOCUSIPCCTRL_CONTROLLERVIRTUAL_H

#include "../CanController.h"

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using std::cout; using std::endl;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

class ControllerVirtual: public CanController {

public:
    explicit ControllerVirtual() {

        m_logFile = std::tmpfile();

        std::cout << "Log file is " << fs::read_symlink(
                fs::path("/proc/self/fd") / std::to_string(fileno(m_logFile))
        ) << '\n';

    }

    int set_protocol(CAN_PROTO protocol) override {

        auto ms_since_epoch = duration_cast<std::chrono::microseconds>(system_clock::now().time_since_epoch()).count();
        fprintf(stderr, "T <%ld> | %s |" "Proto: %d\n", ms_since_epoch, __FUNCTION__, protocol);

        return 0;
    }

    int transaction(unsigned int ecu_address, std::vector<uint8_t> &data) override {

        std::stringstream ss;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(3) << ecu_address << ": " << std::setw(2);
        for (auto d: data) { ss << std::setfill('0') << std::setw(2) << (int) d << " "; }

        auto ms_since_epoch = duration_cast<std::chrono::microseconds>(system_clock::now().time_since_epoch()).count();
        fprintf(stderr, "T <%ld> | " "%s\n", ms_since_epoch, ss.str().c_str());

        return 0;
    }

private:

    std::FILE *m_logFile;
};


#endif //FOCUSIPCCTRL_CONTROLLERVIRTUAL_H
