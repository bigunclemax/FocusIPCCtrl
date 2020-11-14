//
// Created by user on 30.10.2020.
//

#include <sstream>
#include <iomanip>
#include <iostream>
#include <utility>
#include "QControllerEls27.h"

const uint32_t SerialHandler::baud_arr[] = {
        9600,
        19200,
        38400,
        57600,
        115200,
        230400,
        460800,
        500000,
        576000,
        921600,
        1000000,
        1152000,
        1500000,
        2000000,
        2500000,
        3000000,
        3500000,
        4000000 };

const int SerialHandler::baud_arr_sz = sizeof(baud_arr) / sizeof(baud_arr[0]);

static inline void print_buffer(int rdlen, const unsigned char *const buf, int isWrite) {

#ifdef DEBUG_BUFFERS
    std::vector<uint8_t> _str(rdlen+1);
    _str[rdlen] = 0;

    fprintf(stderr,"%s %d:", isWrite? "W" : "R", rdlen);
    /* first display as hex numbers then ASCII */
    for (int i =0; i < rdlen; i++) {
#ifdef DEBUG_BUFFERS_HEX
        fprintf(stderr," 0x%x", buf[i]);
#endif
        if (buf[i] < ' ')
            _str[i] = '.';   /* replace any control chars */
        else
            _str[i] = buf[i];
    }
    fprintf(stderr,"\n    \"%s\"\n\n", _str.data());
#endif
}

SerialHandler::SerialHandler(sControllerSettings init_settings, QObject *parent) :
        QThread(parent),
        m_init_settings(std::move(init_settings)),
        m_portName(m_init_settings.port_name.c_str())
{
}

SerialHandler::~SerialHandler()
{
    m_mutex.lock();
    m_quit = true;
    m_cond.wakeOne();
    m_mutex.unlock();
    wait();
}

int SerialHandler::transaction(int waitTimeout, const std::string &request)
{
    usedBytes.acquire();
    m_mutex.lock();
    const QMutexLocker locker2(&m_mutex2);
    m_waitTimeout = waitTimeout;
    m_request = request;
    if (!isRunning())
        start();
    else
        m_cond.wakeOne();

    m_mutex.unlock();           //FIXME: may cause to race condition
    m_cond2.wait(&m_mutex2);
    return m_transaction_res;
}

void SerialHandler::run()
{

    if (m_portName.isEmpty()) {
        std::cerr << "No port name specified\n";
        usedBytes.release();
        return;
    }

    QSerialPort serial;
    serial.setPortName(m_portName);

    if (!serial.open(QIODevice::ReadWrite)) {
        std::cerr << "Can't open " << m_portName.toStdString() << ", error code " << serial.error() << std::endl;
        return;
    } else if(_init(serial)) {
        std::cerr << "Can't init els device" << std::endl;
        return;
    }

    int currentWaitTimeout = m_waitTimeout;
    std::string currentRequest = m_request;

    while (!m_quit) {

        int transaction_res = serial_transaction(serial, currentRequest, currentWaitTimeout).first;

        m_mutex.lock();     //FIXME: may cause to race condition
        m_cond2.wakeOne();
        m_transaction_res = transaction_res;
        usedBytes.release();
        m_cond.wait(&m_mutex);
        currentWaitTimeout = m_waitTimeout;
        currentRequest = m_request;
        m_mutex.unlock();
    }
}


std::pair<int, std::string>
SerialHandler::serial_transaction(QSerialPort &serial, const std::string &req, int timeout) {

    print_buffer(req.size(), reinterpret_cast<const unsigned char *const>(req.data()), true);

    serial.write(req.c_str());
    if(!serial.waitForBytesWritten()) {
        std::cerr << "waitForBytesWritten timeout\n";
        return {-1, ""};
    }

    uint8_t buffer[4096];
    unsigned response_max_len = sizeof(buffer);
    int idx = 0;
    do {
        if(!serial.waitForReadyRead(timeout)) {
            std::cerr << "waitForReadyRead timeout\n";
            return {-1, ""};
        }
        auto rdlen = serial.read(reinterpret_cast<char *>(&buffer[idx]), response_max_len - idx - 1);
        if (rdlen > 0) {
            idx += rdlen;
            if('>' == buffer[idx-1]  || '>' == buffer[idx-2]) {
                break;
            }
        } else if (rdlen < 0) {
            std::cerr << "Error from read. Read " << rdlen << " bytes\n";
            return {-1, ""};
        } else {  /* rdlen == 0 */
            std::cerr << "Nothing read. EOF?\n";
            return {-1, ""};
        }
        /* repeat read */
    } while (true);

    print_buffer(idx, reinterpret_cast<const unsigned char *const>(buffer), false);

    return {0, std::string(buffer, buffer+idx)};
}

int SerialHandler::_init(QSerialPort& serial) {

    if(!serial.setStopBits(QSerialPort::OneStop))
    {
        throw std::runtime_error(QString(("failed to set stop bits on port %1")).arg(serial.portName()).toLatin1());
    }
    if(!serial.setParity(QSerialPort::NoParity))
    {
        throw std::runtime_error(QString(("failed to set parity bits on port %1")).arg(serial.portName()).toLatin1());
    }
    if(!serial.setFlowControl(QSerialPort::NoFlowControl))
    {
        throw std::runtime_error(QString(("failed to set flow control on port %1")).arg(serial.portName()).toLatin1());
    }

    serial.clear();


    if(m_init_settings.baud) {
        std::cerr << "Use users baud rate: " << m_init_settings.baud << std::endl;
        if(check_baudrate(serial, m_init_settings.baud)) {
            throw std::runtime_error("failed to connect adapter");
        }
    } else {
        std::cerr << "Use auto detected baud rate" << std::endl;
        if(!detect_baudrate(serial)) {
            throw std::runtime_error("failed to connect adapter");
        }
    }

    if(m_init_settings.maximize) {
        std::cerr << "Try maximize baud rate" << std::endl;
        maximize_baudrate(serial);
    }

    std::cerr << "Els baud rate: " << serial.baudRate() << std::endl;

    serial_transaction(serial, "\r");
    serial_transaction(serial, "ATE1\r");   //echo off
    serial_transaction(serial, "ATL0\r");   //linefeeds off
    serial_transaction(serial, "ATS0\r");   //spaces off
    serial_transaction(serial, "STPO\r");   //ATBI Open current protocol.
    serial_transaction(serial, "ATAL\r");   //allow long messages
    serial_transaction(serial, "ATAT0\r");  //disable adaptive timing
    serial_transaction(serial, "ATCAF0\r"); //CAN auto formatting off
    serial_transaction(serial, "ATST01\r"); //Set timeout to hh(13) x 4 ms
    serial_transaction(serial, "ATR0\r");   //Responses on

    serial_transaction(serial, "STI\r");    //Get els description

    return 0;
}

int SerialHandler::check_baudrate(QSerialPort& serial, uint32_t baud) {

    std::cerr << "Check baud: " << baud << std::endl;

    if(!serial.setBaudRate(baud))
    {
        return -1;
    }

    /* clear */
    if(serial_transaction(serial,"?\r", check_baudrate_timeout).first) {
        return -1;
    }

    auto r = serial_transaction(serial,"ATWS\r", check_baudrate_timeout);
    if(r.first) {
        return -1;
    }

    if (r.second.find("ELM327 v1.3a") == std::string::npos) {
        return -1;
    }

    std::cerr << "Baud " << baud << " supported" << std::endl;

    return 0;
}

int SerialHandler::detect_baudrate(QSerialPort& serial) {
    for(unsigned int i : baud_arr) {
        if(!check_baudrate(serial, i))
            return (int)i;
    }
    return 0;
}

int SerialHandler::set_baudrate(QSerialPort &serial, uint32_t baud) {

    auto curr_baud = serial.baudRate();

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    int stbr_str_sz = snprintf(io_buff, io_buff_max_len, "STBR %d\r", baud);
    if(stbr_str_sz < 0) {
        return -1;
    }

    /* Host sends STBR */
    serial.write(io_buff);


    unsigned rcv_sz;
    if(serial.waitForReadyRead()) {
        rcv_sz = serial.read(io_buff, io_buff_max_len);
        while (serial.waitForReadyRead(50))
            rcv_sz += serial.read(io_buff+rcv_sz, io_buff_max_len-rcv_sz);
    } else {
        return -1;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        return -1;
    }

    /* Host: switch to new baud rate */
    if(!serial.setBaudRate(baud))
    {
        return -1;
    }

    if(serial.waitForReadyRead()) {
        rcv_sz = serial.read(io_buff, io_buff_max_len);
        while (serial.waitForReadyRead(100))
            rcv_sz += serial.read(io_buff+rcv_sz, io_buff_max_len-rcv_sz);
    } else {
        goto cleanup;
    }

    /* Host: received a valid STI string? */
    if(strstr(io_buff, "STN1170 v3.3.1") == nullptr) {
        goto cleanup;
    }

    serial.write("\r");

    if(serial.waitForReadyRead()) {
        rcv_sz = serial.read(io_buff, io_buff_max_len);
        while (serial.waitForReadyRead(100))
            rcv_sz += serial.read(io_buff+rcv_sz, io_buff_max_len-rcv_sz);
    } else {
        goto cleanup;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        goto cleanup;
    }

    return 0;

cleanup:
    std::this_thread::sleep_for(std::chrono::milliseconds(set_baudrate_timeout));

    if(serial.setBaudRate(curr_baud))
        serial_transaction(serial, "?\r");

    return -1;
}

int SerialHandler::maximize_baudrate(QSerialPort &serial) {

    /* set baudrate timeout in ms */
    std::string str = "STBRT " + std::to_string(set_baudrate_timeout) + "\r";
    if(serial_transaction(serial, str).first) {
        return -1;
    }

    //TODO: find pos in sorted arr
    auto baud = serial.baudRate();
    int i=0;
    while (baud != baud_arr[i]) {
        if(++i > baud_arr_sz) {
            return 0; //already maximized
        }
    }

    for(int j = i + 1; j < baud_arr_sz; ++j) {
        if(set_baudrate(serial, baud_arr[j])) { //if ok, goes next
            std::cerr << "Baud rate " << baud_arr[j] << " not supported" << std::endl;
            continue;
        }
        std::cerr << "Baud rate " << baud_arr[j] << " supported" << std::endl;
        baud = baud_arr[j];
    }

    return baud;
}

QControllerEls27::QControllerEls27(sControllerSettings init_settings)
    : comPort(std::move(init_settings))
{}

int QControllerEls27::control_msg(const std::string &req) {
    comPort.transaction(3000, req + '\r');
    return 0; //TODO: add return code
}

int QControllerEls27::send_data(std::vector<uint8_t> &data) {
    auto tx_size = data.size() > CAN_frame_sz ? 8 : data.size();
    std::string io_buff;
    io_buff.resize(tx_size * 2 + 1);
    io_buff[tx_size * 2] = '\r';
    hex2ascii(data.data(), tx_size, io_buff.data());
    comPort.transaction(3000, io_buff);
    return 0; //TODO: add return code
}

int QControllerEls27::set_ecu_address(unsigned int ecu_address) {
    std::stringstream ss;
    ss << "ATSH" << std::hex << std::setfill('0') << std::setw(3) <<  ecu_address;
    control_msg(ss.str());
    return 0;
}

int QControllerEls27::set_protocol(CanController::CAN_PROTO protocol) {
    if(CAN_MS == protocol)
        control_msg("STP53");   //ISO 15765, 11-bit Tx, 125kbps, DLC=8

    return 0;
}

int QControllerEls27::transaction(unsigned int ecu_address, std::vector<uint8_t> &data) {

    auto res = set_ecu_address(ecu_address);
    res = send_data(data);

    return 0;
}

//if(ecu_address == 0x3a || ecu_address == 0x80) {
//printf("0x%03x ", ecu_address);for(auto d : data) {printf("0x%02x ", d);}printf("\n");
//}