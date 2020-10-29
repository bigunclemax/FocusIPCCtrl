//
// Created by user on 22.08.2020.
//

//#define OPTIMIZATION 1
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <sys/select.h>

#ifdef PRINT_BUFFERS
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
#endif

int baud2int(speed_t baud)
{
    switch (baud) {
        case B9600:
            return 9600;
        case B19200:
            return 19200;
        case B38400:
            return 38400;
        case B57600:
            return 57600;
        case B115200:
            return 115200;
        case B230400:
            return 230400;
        case B460800:
            return 460800;
        case B500000:
            return 500000;
        case B576000:
            return 576000;
        case B921600:
            return 921600;
        case B1000000:
            return 1000000;
        case B1152000:
            return 1152000;
        case B1500000:
            return 1500000;
        case B2000000:
            return 2000000;
        case B2500000:
            return 2500000;
        case B3000000:
            return 3000000;
        case B3500000:
            return 3500000;
        case B4000000:
            return 4000000;
        default:
            return -1;
    }
}

int els27_open_port(const char *port_name) {

    int fd = open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", port_name, strerror(errno));
    }

    return fd;
}

//static int set_interface_attribs(int tty_fd, speed_t baudrate)
//{
//    struct termios tty = {0};
//
//    cfsetispeed(&tty, baudrate);
//    cfsetospeed(&tty, baudrate);
//
//    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
//    // disable IGNBRK for mismatched speed tests; otherwise receive break
//    // as \000 chars
//    tty.c_iflag &= ~IGNBRK;         // disable break processing
//    tty.c_lflag = 0;                // no signaling chars, no echo,
//    // no canonical processing
//    tty.c_oflag = 0;                // no remapping, no delays
//    tty.c_cc[VMIN]  = 0;            // read doesn't block
//    tty.c_cc[VTIME] = 2;            // 0.5 seconds read timeout
//
//    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
//
//    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
//    // enable reading
//    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
//    tty.c_cflag &= ~CSTOPB;
//    tty.c_cflag &= ~CRTSCTS;
//
//    tcsetattr(tty_fd, TCSANOW, &tty);
//
//    return 0;
//}

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

//    tty.c_lflag |= ICANON | ISIG;  /* canonical input */
    tty.c_lflag &= ~(ECHO | ECHOE | ECHONL | IEXTEN);

//    tty.c_iflag &= ~IGNCR;  /* preserve carriage return */
    tty.c_iflag &= ~INPCK;
    tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | IMAXBEL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);   /* no SW flowcontrol */

    tty.c_oflag &= ~OPOST;

//    tty.c_cc[VEOL] = 0;
//    tty.c_cc[VEOL2] = 0;
//    tty.c_cc[VEOF] = 0x04;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int els27_write(int tty_fd, const char *request, unsigned request_len) {
#ifdef PRINT_BUFFERS
    printf("Write %d: %s\n", request_len, request);
#endif
    ssize_t wlen = write(tty_fd, request, request_len);
    if (wlen != request_len) {
        printf("Error from write. Written %ld Error: %s", wlen, strerror(errno));
        return -1;
    }
//    tcdrain(tty_fd);    /* delay for output */

    return 0;
}

int serial_read(int tty_fd, char *response, unsigned* response_len) {
    int idx = 0;
    unsigned response_max_len = *response_len;
    fd_set set;
    FD_ZERO(&set); /* clear the set */
    FD_SET(tty_fd, &set); /* add our file descriptor to the set */

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int rv = select(tty_fd + 1, &set, NULL, NULL, &timeout);
    if(rv == -1) {
        perror("select\n"); /* an error accured */
        return -1;
    } else if(rv == 0) {
        printf("Error from read. Read timeout\n"); /* a timeout occured */
        return -1;
    }

    /* simple canonical input */
    do {
        ssize_t rdlen = read(tty_fd, &response[idx], response_max_len - idx - 1);
        if (rdlen > 0) {
            idx += rdlen;
        } else if (rdlen < 0) {
            printf("Error from read. Readed %ld Error: %s", rdlen, strerror(errno));
            return -1;
        } else {  /* rdlen == 0 */
            printf("Nothing read. EOF?");
#ifdef PRINT_BUFFERS
            *response_len = idx;
            print_buffer(*response_len, response, 0);
#endif
//            return 0;
        }
        /* repeat read */
    } while (1);
}

int els27_transaction(int tty_fd, const char *request, unsigned request_len, char *response, unsigned *response_len) {
#ifdef PRINT_BUFFERS
    print_buffer(request_len, request, 1);
#endif
    ssize_t wlen = write(tty_fd, request, request_len);
    if (wlen != request_len) {
        printf("Error from write. Written %ld Error: %s", wlen, strerror(errno));
        return -1;
    }
#if OPTIMIZATION
    tcdrain(tty_fd);    /* delay for output */
#endif
    int idx = 0;
    unsigned response_max_len = *response_len;
    fd_set set;
    FD_ZERO(&set); /* clear the set */
    FD_SET(tty_fd, &set); /* add our file descriptor to the set */
#if OPTIMIZATION
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int rv = select(tty_fd + 1, &set, NULL, NULL, &timeout);
    if(rv == -1) {
        perror("select\n"); /* an error accured */
        return -1;
    } else if(rv == 0) {
        printf("Error from read. Read timeout\n"); /* a timeout occured */
        return -1;
    }
#endif
    /* simple canonical input */
    do {
        ssize_t rdlen = read(tty_fd, &response[idx], response_max_len - idx - 1);
        if (rdlen > 0) {
            idx += rdlen;
            if('>' == response[idx-1]  || '>' == response[idx-2]) {
                *response_len = idx;
#ifdef PRINT_BUFFERS
                print_buffer(*response_len, response, 0);
#endif
                break;
            }
        } else if (rdlen < 0) {
            printf("Error from read. Reared %ld Error: %s", rdlen, strerror(errno));
            return -1;
        } else {  /* rdlen == 0 */
            printf("Nothing read. EOF?\n");
            return -1;
        }
        /* repeat read */
    } while (1);

    return 0;
}

speed_t baud_arr[] = {        B38400,
                              B57600,
                              B115200,
                              B230400,
                              B460800,
                              B500000,
                              B576000,
                              B921600,
                              B1000000,
                              B1152000,
                              B1500000,
                              B2000000,
                              B2500000,
                              B3000000,
                              B3500000,
                              B4000000 };

static int baud_arr_sz = sizeof(baud_arr) / sizeof(baud_arr[0]);

int els27_check_baudrate(int tty_fd, speed_t baud) {

    printf("Check baud: %d\n", baud2int(baud));

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    if(set_interface_attribs(tty_fd, baud)) {
        printf("Can't set baudrate %d to port\n", baud2int(baud));
        return -1;
    }

    unsigned rcv_sz = io_buff_max_len;
    if(els27_transaction(tty_fd, "?\r", sizeof("?\r"), io_buff, &rcv_sz)) {
        printf("Can't set baudrate: %d\n", baud2int(baud));
        return -1;
    }

    rcv_sz = io_buff_max_len;
    if(els27_transaction(tty_fd, "ATWS\r", sizeof("ATWS\r"), io_buff, &rcv_sz)) {
        printf("Can't set baudrate: %d\n", baud2int(baud));
        return -1;
    }

    if(strstr(io_buff, "ELM327 v1.3a") == NULL) {
        return -1;
    }

    return 0;
}

speed_t els27_detect_baudrate(int tty_fd) {

    for(int i=0; i < baud_arr_sz; ++i) {
        if(!els27_check_baudrate(tty_fd, baud_arr[i]))
            return baud_arr[i];
    }
    return 0;
}

int els27_set_baudrate(int tty_fd, speed_t baud) {

    printf("Try to set baudrate: %d\n", baud2int(baud));

    struct termios tty = {0};
    tcgetattr(tty_fd, &tty);
    speed_t curr_baud = cfgetispeed(&tty);

    unsigned rcv_sz =0;
    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    int stbr_str_sz = snprintf(io_buff, io_buff_max_len, "STBR %d\r", baud2int(baud));
    if(stbr_str_sz < 0) {
        return -1;
    }

    /* Host sends STBR */
    if(els27_write(tty_fd, io_buff, stbr_str_sz)) {
        return -1;
    }

    rcv_sz = io_buff_max_len;
    if(serial_read(tty_fd, io_buff, &rcv_sz)) {
        return -1;
    }

    if(strstr(io_buff, "OK") == NULL) {
        return -1;
    }

    /* Host: switch to new baud rate */
    if(set_interface_attribs(tty_fd, baud)) {
        printf("Can't set baudrate %d to port\n", baud2int(baud));
        return -1;
    }

    rcv_sz = io_buff_max_len;
    if(serial_read(tty_fd, io_buff, &rcv_sz)) {
        goto cleanup;
    }

    /* Host: received a valid STI string? */
    if(strstr(io_buff, "STN1170 v3.3.1") == NULL) {
        goto cleanup;
    }

    if(els27_write(tty_fd, "\r", 1)) {
        goto cleanup;
    }

    rcv_sz = io_buff_max_len;
    if(serial_read(tty_fd, io_buff, &rcv_sz)) {
        goto cleanup;
    }

    if(strstr(io_buff, "OK") == NULL) {
        goto cleanup;
    }

    return 0;

cleanup:

    if(set_interface_attribs(tty_fd, curr_baud)) {
        printf("Can't set baudrate %d to port\n", baud2int(baud));
    }

    return -1;
}

speed_t els27_maximize_speed(int tty_fd) {

    const unsigned io_buff_max_len = 1024; //alloc 1kb buffer
    char io_buff[io_buff_max_len];

    speed_t baud = els27_detect_baudrate(tty_fd);
    if(baud == 0) {
        return 0;
    }

    unsigned rcv_sz = io_buff_max_len;
    if(els27_transaction(tty_fd, "STBRT 500\r", sizeof("STBRT 500\r"), io_buff, &rcv_sz)) {
        return -1;
    }

    int i=0;
    while (baud != baud_arr[i]) {
        if(++i > baud_arr_sz) {
            return 0;
        }
    }

    for(int j = i + 1; j < baud_arr_sz; ++j) {
        if(els27_set_baudrate(tty_fd, baud_arr[j])) { //if ok, goes next
            continue;
        }
        baud = baud_arr[j];
    }

    return baud;
}

int els27_setup_baudrate(int tty_fd, speed_t baud) {
    if(!els27_detect_baudrate(tty_fd) || els27_set_baudrate(tty_fd, baud)) {
        return -1;
    }
    return 0;
}