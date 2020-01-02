#include <linux/serial_core.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <asm-generic/termbits.h>
extern int tcflush(int, int);

char *cdef_string =
    "int sync_futaba(int);"
    "int open_futaba(const char *);"
    "int read_futaba_packet(int, uint8_t *);"
    "int skip_futaba_packet(int);"
    "int write_futaba_packet(int, uint8_t *);"
    ;

#define SBUS_BAUD 100000
#define FRAME_START 0x0f
#define FRAME_LEN 25

void sync_futaba(int fd)
{
    struct termios options;
    
    if (ioctl(fd, TCGETS2, &options) < 0)
	err(1, "Can't get termios2 attributes");
    
    uint8_t buf[FRAME_LEN];
   
    while (1) {
	options.c_cc[VMIN] = 1;

	if (ioctl(fd, TCSETS2, &options) != 0)
	    err(1, "Can't set termios attributes");

	// Scan for frame start and skip rest of frame.
	while(1) {
	    if (read(fd, buf, 1) < 0)
		err(1, "Can't set read futaba");
	    if (*buf == FRAME_START) {
		for (int i = 0; i < 24; i++)
		    if (read(fd, buf, 1) < 0)
			err(1, "Can't read futaba");
		break;
	    }
	}
	options.c_cc[VMIN] = FRAME_LEN;
    
	if (ioctl(fd, TCSETS2, &options) != 0)
	    err(1, "Can't set termios attributes");

	// Be sure frame start wasn't in the data.
	// Other bytes are assumed to move around a bit.
	for (int i = 0; i < 20; i++) {
	    if (read(fd, buf, FRAME_LEN) < 0)
		err(1, "Can't read futaba");
	    if (*buf != FRAME_START)
		break;
	}
	if (*buf == FRAME_START)
	    return;
    }
}

// Open a serial device and try to set (custom) S.Bus baud.
int open_futaba(char *name)
{
    struct termios2 options;

    int fd = open(name, O_RDWR | O_NOCTTY /* | O_NONBLOCK*/);
    if (fd < 0)
	err(1, "Can't open S.Bus device");
    
    if (ioctl(fd, TCGETS2, &options) < 0)
	err(1, "Can't get termios2 attributes");
    
    options.c_cflag |= CS8 | CLOCAL | PARENB | CSTOPB | CREAD;
    options.c_cflag &= ~(PARODD | CRTSCTS | HUPCL);

    options.c_lflag &= ~(ECHO | ISIG | ICANON | IEXTEN);

    options.c_iflag &= ~(IXON | IUCLC | INLCR | ICRNL | ISTRIP |
			 IGNBRK | BRKINT | PARMRK | IGNCR );
    options.c_iflag |= INPCK | IGNPAR;
    
    options.c_oflag &= ~OPOST;
    // Set non-std matched baud.
    options.c_ospeed = SBUS_BAUD;
    options.c_cflag &= ~(CBAUD | CBAUD<<IBSHIFT) ;
    options.c_cflag |= BOTHER | B0<<IBSHIFT;
    // Block until at least one character becomes available.
    options.c_cc[VMIN] = FRAME_LEN;
    options.c_cc[VTIME] = 0;

    if (ioctl(fd, TCSETS2, &options) != 0)
	err(1, "Can't set termios attributes");

    tcflush(fd, TCIOFLUSH);

    return fd;
}

int read_futaba_packet(int fd, uint8_t *buf)
{
    // Assume blocking
    read(fd, buf, FRAME_LEN);
    // True if packet ends are currect;
    return buf[0] == 0x0f && buf[24] == 0x00;
}

int skip_futaba_packet(int fd)
{
    uint8_t buf[FRAME_LEN];
    return read_futaba_packet(fd, buf);
}

int write_futaba_packet(int fd, uint8_t *buf)
{
    return write(fd, buf, FRAME_LEN);
}

// Do we want some fixed interpacket delay routines?
