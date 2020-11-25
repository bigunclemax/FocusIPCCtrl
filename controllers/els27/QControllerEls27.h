//
// Created by user on 30.10.2020.
//

#ifndef GUI_QCONTROLLERELS27_H
#define GUI_QCONTROLLERELS27_H

#include <QSerialPort>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>
#include "../CanController.h"

struct sControllerSettings {
    std::string port_name;
    uint32_t    baud;
    bool        maximize;
};

class SerialHandler : public QThread {

    Q_OBJECT

public:
    explicit SerialHandler(sControllerSettings init_settings, QObject *parent = nullptr);
    ~SerialHandler() override;

    int transaction(int waitTimeout, const std::string &request);
    [[nodiscard]] bool isElm327() const { return m_isElm327; }

private:
    void run() override;
    int _init(QSerialPort& serial);
    int check_baudrate(QSerialPort& serial, uint32_t baud);
    int detect_baudrate(QSerialPort& serial);
    int set_baudrate(QSerialPort &serial, uint32_t baud);
    int maximize_baudrate(QSerialPort &serial);
    std::pair<int, std::string> serial_transaction(QSerialPort &serial, const std::string &req, int timeout = 1000);

    static constexpr int check_baudrate_timeout = 3000; ///< max els response timeout in ms
    static constexpr int set_baudrate_timeout = 1000;   ///< baud rate switch timeout in ms

    static const uint32_t   baud_arr[];
    static const int        baud_arr_sz;

    sControllerSettings     m_init_settings;
    bool                    m_isElm327 = false;
    QString     m_portName;
    std::string m_request;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    QMutex m_mutex2;
    QWaitCondition m_cond;
    QWaitCondition m_cond2;
    bool m_quit = false;
    QSemaphore usedBytes = QSemaphore(1);
    int m_transaction_res = 0;
};

class QControllerEls27: public CanController {

public:

    explicit QControllerEls27(sControllerSettings init_settings);
    QControllerEls27(QControllerEls27 const &) = delete;

    int init() override { return 0; };
    int send_data(std::vector<uint8_t> &data) override;
    int transaction(unsigned ecu_address, std::vector<uint8_t> &data) override;
    void set_logger(CanLogger *logger) override;
    void remove_logger() override;
private:
    int set_protocol(CAN_PROTO protocol) override;
    int set_ecu_address(unsigned ecu_address) override;
    int control_msg(const std::string &req);
    inline static void hex2ascii(const uint8_t* bin, unsigned int binsz, char* result)
    {
        unsigned char     hex_str[]= "0123456789ABCDEF";

        for (auto i = 0; i < binsz; ++i)
        {
            result[i * 2 + 0] = (char)hex_str[(bin[i] >> 4u) & 0x0Fu];
            result[i * 2 + 1] = (char)hex_str[(bin[i]      ) & 0x0Fu];
        }
    };

    SerialHandler   comPort;
    std::mutex      mutex;
    CanLogger*      m_logger{};
    std::atomic<bool> is_logger_set = false;
};

#endif //GUI_QCONTROLLERELS27_H