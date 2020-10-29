//
// Created by user on 22.08.2020.
//

#ifndef IPCFLASHER_ELS27_H
#define IPCFLASHER_ELS27_H

#include <termios.h>

int baud2int(speed_t baud);

int els27_open_port(const char *port_name);
speed_t els27_detect_baudrate(int tty_fd);
speed_t els27_maximize_speed(int tty_fd);
int els27_check_baudrate(int tty_fd, speed_t baud);
int els27_setup_baudrate(int tty_fd, speed_t baud); //TODO: deprecated
int els27_transaction(int tty_fd, const char *request, unsigned request_len, char *response, unsigned *response_len);
int els27_write(int tty_fd, const char *request, unsigned request_len);

#endif //IPCFLASHER_ELS27_H
