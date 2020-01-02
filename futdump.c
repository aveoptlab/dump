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

#define SBUS_BAUD 100000
#define FRAME_LEN 25

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


int main(int argc, char *argv[])
{
    if (argc <= 1)
	return 0;

    int fd = open_futaba(argv[1]);

    uint8_t buf[25];

    int count = 0;
    
    while (1) {
	int ac = read(fd, buf, FRAME_LEN);

	if (ac < 0)
	    err(1, NULL);

	if (ac != 25)
	    errx(1, "Insufficient bytes read.");


	printf("%06X: ", count = (count + 1) & 0x0FFFFFF);
	for (int i=0; i < 25; i++)
	    printf("%02X ", buf[i]);
	puts("");
    }
    
    return 0;
}
