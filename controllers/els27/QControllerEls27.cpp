//
// Created by user on 30.10.2020.
//

#include <sstream>
#include <iomanip>
#include <iostream>
#include <utility>
#include "QControllerEls27.h"

const uint32_t SerialHandler::baud_arr[] = {
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

#define PRINT_HEX
static void print_buffer(int rdlen, const unsigned char *const buf, int isWrite) {

    unsigned char _str[rdlen+1];
    _str[rdlen] = 0;

    printf("%s %d:", isWrite? "W" : "R", rdlen);
    /* first display as hex numbers then ASCII */
    for (int i =0; i < rdlen; i++) {
#ifdef PRINT_HEX
        printf(" 0x%x", buf[i]);
#endif
        if (buf[i] < ' ')
            _str[i] = '.';   /* replace any control chars */
        else
            _str[i] = buf[i];
    }
    printf("\n    \"%s\"\n\n", _str);
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

void SerialHandler::transaction(int waitTimeout, const std::string &request)
{
    usedBytes.acquire();
    const QMutexLocker locker(&m_mutex);
    m_waitTimeout = waitTimeout;
    m_request = request;
    if (!isRunning())
        start();
    else
        m_cond.wakeOne();
}

void SerialHandler::run()
{

    bool currentPortNameChanged = false;

    m_mutex.lock();
    QString currentPortName;
    if (currentPortName != m_portName) {
        currentPortName = m_portName;
        currentPortNameChanged = true;
    }

    int currentWaitTimeout = m_waitTimeout;
    std::string currentRequest = m_request;
    m_mutex.unlock();
    QSerialPort serial;

    if (currentPortName.isEmpty()) {
        std::cerr << "No port name specified\n";
        usedBytes.release();
        return;
    }

    while (!m_quit) {
        if (currentPortNameChanged) {
            serial.close();
            serial.setPortName(currentPortName);

            if (!serial.open(QIODevice::ReadWrite)) {
                std::cerr << "Can't open " << m_portName.toStdString() << ", error code " << serial.error() << std::endl;
                usedBytes.release();
                return;
            }

            if(_init(serial)) {
                std::cerr << "Can't init els device" << std::endl;
                usedBytes.release();
                return;
            }
        }
        // write request
        if(serial_transaction(serial, currentRequest, currentWaitTimeout).first) {
            std::cerr << "serial_transaction error" << std::endl;
        }
        usedBytes.release();
#ifdef _NO
        const QByteArray requestData = currentRequest.toUtf8();
        serial.write(requestData);
        if (serial.waitForBytesWritten(m_waitTimeout)) {
            // read response
            if (serial.waitForReadyRead(currentWaitTimeout)) {
                QByteArray responseData = serial.readAll();
                while (serial.waitForReadyRead(10))
                    responseData += serial.readAll();

                const QString response = QString::fromUtf8(responseData);
//                emit this->response(response);
            } else {
                std::cerr << "Wait read response timeout \n";
//                emit timeout(tr("Wait read response timeout %1")
//                                     .arg(QTime::currentTime().toString()));
            }
        } else {
            std::cerr << "Wait write request timeout \n";
//            emit timeout(tr("Wait write request timeout %1")
//                                 .arg(QTime::currentTime().toString()));
        }
#endif
        m_mutex.lock();
        m_cond.wait(&m_mutex);
        if (currentPortName != m_portName) {
            currentPortName = m_portName;
            currentPortNameChanged = true;
        } else {
            currentPortNameChanged = false;
        }
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
        if(test_baudrate(serial, m_init_settings.baud)) {
            throw std::runtime_error("failed to connect adapter");
        }
    } else {
        if(!detect_baudrate(serial)) {
            throw std::runtime_error("failed to connect adapter");
        }
    }
#ifdef _DIS
    if(init_settings.maximize) {
        maximize_baudrate();
    }
#endif

    std::cerr << "Els baud rate: " << serial.baudRate() << std::endl;

    serial_transaction(serial, "\r");
    serial_transaction(serial, "ATE1\r"); //echo off
    serial_transaction(serial, "ATL0\r"); //linefeeds off
    serial_transaction(serial, "ATS0\r"); //spaces off
    serial_transaction(serial, "STPO\r");  //ATBI Open current protocol.
    serial_transaction(serial, "ATAL\r");  //allow long messages
    serial_transaction(serial, "ATAT0\r"); //disable adaptive timing
    serial_transaction(serial, "ATCAF0\r");//CAN auto formatting off
    serial_transaction(serial, "ATST01\r");  //Set timeout to hh(13) x 4 ms
    serial_transaction(serial, "ATR0\r");    //Responses on
    return 0;
}

int SerialHandler::test_baudrate(QSerialPort& serial, uint32_t baud) {

    printf("Check baud: %d\n", baud);

    if(!serial.setBaudRate(baud))
    {
        return -1;
    }

    /* clear */
    if(serial_transaction(serial,"?\r", 500).first) {
        return -1;
    }

    auto r = serial_transaction(serial,"ATWS\r", 500);
    if(r.first) {
        return -1;
    }

    if (r.second.find("ELM327 v1.3a") == std::string::npos) {
        return -1;
    }

    return 0;
}

int SerialHandler::detect_baudrate(QSerialPort& serial) {
    for(unsigned int i : baud_arr) {
        if(!test_baudrate(serial, i))
            return (int)i;
    }
    return 0;
}

QControllerEls27::QControllerEls27(sControllerSettings init_settings)
    : comPort(std::move(init_settings))
{}

int QControllerEls27::control_msg(const std::string &req) {
    comPort.transaction(3000, req + '\r');
    return 0; //TODO: add return code
}

int QControllerEls27::RAW_transaction(std::vector<uint8_t> &data) {
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

int QControllerEls27::set_baudrate(uint32_t baud) {

#ifdef _DIS
    auto curr_baud = baudRate();

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    int stbr_str_sz = snprintf(io_buff, io_buff_max_len, "STBR %d\r", baud);
    if(stbr_str_sz < 0) {
        return -1;
    }

    /* Host sends STBR */
    write(io_buff);


    waitForReadyRead();
    unsigned rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        return -1;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        return -1;
    }

    /* Host: switch to new baud rate */
    if(!setBaudRate(baud))
    {
        return -1;
    }

    waitForReadyRead();
    rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        goto cleanup;
    }

    /* Host: received a valid STI string? */
    if(strstr(io_buff, "STN1170 v3.3.1") == nullptr) {
        goto cleanup;
    }

    write("\r");

    waitForReadyRead();
    rcv_sz = read(io_buff, io_buff_max_len);
    if(rcv_sz <= 0) {
        goto cleanup;
    }

    if(strstr(io_buff, "OK") == nullptr) {
        goto cleanup;
    }

    return 0;

cleanup:

    setBaudRate(curr_baud);
#endif
    return -1;
}

int QControllerEls27::maximize_baudrate() {

#ifdef _DIS
    /* set baudrate timeout in ms */
    if(serial_transaction("STBRT 1000\r").first) {
        return -1;
    }

    //TODO: find pos in sorted arr
    auto baud = baudRate();
    int i=0;
    while (baud != baud_arr[i]) {
        if(++i > baud_arr_sz) {
            return 0; //already maximized
        }
    }

    for(int j = i + 1; j < baud_arr_sz; ++j) {
        if(set_baudrate(baud_arr[j])) { //if ok, goes next
            continue;
        }
        baud = baud_arr[j];
    }

    return baud;
#endif
}