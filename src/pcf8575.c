#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
// #include <sys/types.h>
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "tools.h"
#include "pcf8575.h"


void user_delay_ms(uint32_t period)
{
  usleep(period*1000);
}

int8_t user_i2c_read(int fd, uint8_t *data, uint16_t len)
{
  if (read(fd, data, len) != len) {
//    user_delay_ms(50);
    return -1;
  }
//  user_delay_ms(50);
  return 0;
}

int8_t user_i2c_write(int fd, uint8_t *data, uint16_t len)
{
  if (write(fd, data, len) != len) {
//    user_delay_ms(50);
    return -1;
  }
//  user_delay_ms(50);
  return 0;
}

uint16_t getmask(uint8_t pin) {

  uint16_t mask = 1;

  if ((pin < 1) || (pin > 16)) {
    return 0;
  }
  mask = (uint16_t)1 << (pin-1);

  return mask;

}

int8_t buf16Read(struct pcf8575_dev * dev, uint16_t * buf16) {

  int8_t rslt;
  u8     buf8[2];

  if ((rslt = user_i2c_read(dev->iic_fd, buf8, 2)) != 0) {
//    fprintf(stderr, "pcf8575_buf16Read: Read error\n");
    return -1;
  }
#ifdef BIG_ENDIAN
  ((u8 *)buf16)[0] = buf8[0];
  ((u8 *)buf16)[1] = buf8[1];
#else
  ((u8 *)buf16)[0] = buf8[1];
  ((u8 *)buf16)[1] = buf8[0];
#endif

  return 0;

}
int8_t pinRead(struct pcf8575_dev * dev, uint8_t pin) {

  uint16_t mask;
  uint16_t data;
  int8_t   rslt;
  if ((rslt = buf16Read(dev, &data)) != 0) {
    return -1;
  }
  if ((mask = getmask(pin)) == 0) return -2;
  if ((mask & data) == 0) {
    return LOW;
  } else {
    return HIGH;
  }
}

int8_t pinWrite(struct pcf8575_dev * dev, uint8_t pin, uint8_t value) {

  uint16_t mask;
  uint16_t data;
  int8_t   rslt;

  u8 buf8[2];

  if ((mask = getmask(pin)) == 0) return -2;
  //
  // check if the pin in OUTPUT mode ?
  // pin_mode 0 - INPUT, 1 - OUTPUT
  if ((mask & dev->pin_mode) == 0) {
    return -2;
  }
  if (value == LOW) {
    dev->wr_buffer16 &= (~mask);
  } else {
    dev->wr_buffer16 |= mask;
  }
#ifdef BIG_ENDIAN
  buf8[0] = ((u8 *)&(dev->wr_buffer16))[0];
  buf8[1] = ((u8 *)&(dev->wr_buffer16))[1];
#else
  buf8[0] = ((u8 *)&(dev->wr_buffer16))[1];
  buf8[1] = ((u8 *)&(dev->wr_buffer16))[0];
#endif
  if ((rslt = user_i2c_write(dev->iic_fd, buf8, 2)) != 0) {
//    fprintf(stderr, "pcf8575_pinWrite: Write error\n");
//    dev->rw_buffer = data;		//  restore buffer
    return -1;
  }
//  dev->rw_buffer = data;
  return 0;
}

void setPinMode(struct pcf8575_dev * dev, uint8_t pin, uint8_t mode) {
// pin Nr 1..16

  uint16_t mask;

    // #define INPUT  0
    // #define OUTPUT 1
    //
    // #define LOW  0
    // #define HIGH 1


  if ((mask = getmask(pin)) == 0) {
    return;
  }
  // pin_mode 0 - INPUT, 1 - OUTPUT
  if (mode == INPUT) {
    dev->pin_mode |= mask;
    pinWrite(dev, pin, HIGH);
    dev->pin_mode &= ~mask;
  } else if (mode == OUTPUT) {
    dev->pin_mode |= mask;
  } else {
    return;
  }
    
}


struct pcf8575_dev * pcf8575_init(const char *iic_dev, const uint8_t iic_addr) {

  int fd;
  int ret;
  int8_t rslt;
  uint8_t buf8[2];

  struct pcf8575_dev *dev;

  if ((fd = open(iic_dev, O_RDWR)) < 0) {
    fprintf(stderr, "pcf8575_init: Failed to open the i2c bus %s\n", iic_dev);
    return NULL;
  }
  if ((ret = ioctl(fd, I2C_SLAVE, iic_addr)) < 0) {
    fprintf(stderr, "pcf8575_init: Failed to acquire bus access and/or talk to slave at %d. ret=%d\n", iic_addr, ret);
    close(fd);
    return NULL;
  }

  user_delay_ms(50);

  if ((dev = (struct pcf8575_dev*) malloc(sizeof(struct pcf8575_dev))) == NULL) {
    fprintf(stderr, "pcf8575_init: No memory\n");
    close(fd);
    return NULL;
  }
  dev->iic_fd = fd;
//  dev->iic_addr = iic_addr;
  dev->pin_mode = 0xffff;		// 0 - INPUT, 1 - OUTPUT
  					// Initial mode to be INPUT for all pins.
					//   it will be set up below...
  dev->wr_buffer16 = 0xffff;
#ifdef BIG_ENDIAN
  buf8[0] = ((u8 *)&(dev->wr_buffer16))[0];
  buf8[1] = ((u8 *)&(dev->wr_buffer16))[1];
#else
  buf8[0] = ((u8 *)&(dev->wr_buffer16))[1];
  buf8[1] = ((u8 *)&(dev->wr_buffer16))[0];
#endif
  if ((rslt = user_i2c_write(dev->iic_fd, buf8, 2)) != 0) {	// set HIGH Level for all pins
      fprintf(stderr, "pcf8575_init: Write error\n");
      close(dev->iic_fd);
      free(dev);
      return NULL;
  }
  dev->pin_mode = 0x0000;		// Initial mode to be INPUT for all pins.
//  user_delay_ms(50);
  return dev;

}

void pcf8575_close(struct pcf8575_dev *dev) {

  close(dev->iic_fd);
  free(dev);
  return;
}
