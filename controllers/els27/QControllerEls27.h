//
// Created by user on 30.10.2020.
//

#ifndef GUI_QCONTROLLERELS27_H
#define GUI_QCONTROLLERELS27_H

#include <QSerialPort>
#include "../CanController.h"

class QControllerEls27: public CanController, public QSerialPort {

public:

    struct settings {
        std::string port_name;
        uint32_t    baud;
        bool        maximize;
    };

    explicit QControllerEls27(settings init_settings);
    QControllerEls27(QControllerEls27 const &) = delete;

    int init() override;
    int set_ecu_address(unsigned ecu_address) override;
    int set_protocol(CAN_PROTO protocol) override;

    void RAW_transaction(std::vector<uint8_t> &data) override;
    int maximize_baudrate();
    int detect_baudrate();

private:
    int set_baudrate(uint32_t baud);
    int test_baudrate(uint32_t baud);
    std::pair<int, std::string> serial_transaction(const std::string &req);
    void control_msg(const std::string &req);
    inline static void hex2ascii(const uint8_t* bin, unsigned int binsz, char* result)
    {
        unsigned char     hex_str[]= "0123456789ABCDEF";

        for (auto i = 0; i < binsz; ++i)
        {
            result[i * 2 + 0] = (char)hex_str[(bin[i] >> 4u) & 0x0Fu];
            result[i * 2 + 1] = (char)hex_str[(bin[i]      ) & 0x0Fu];
        }
    };

    settings init_settings;

    static const uint32_t baud_arr[];
    static const int baud_arr_sz;
};

#endif //GUI_QCONTROLLERELS27_H