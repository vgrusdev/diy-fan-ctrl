#ifndef PCF8575_H
#define PCF8575_H

#include <stdio.h>
#include <stdint.h>


// PinMode  
#define INPUT  0
#define OUTPUT 1

#define LOW  0
#define HIGH 1


struct pcf8575_dev {
        /*! iic File Descriptor */
        int iic_fd;
        /*! iic Address */
//        uint8_t iic_addr;
        uint16_t pin_mode;               // 0 - INPUT, 1 - OUTPUT
//	union {
        //  uint16_t rw_buffer;
	//  uint8_t  data[2];
	  uint16_t wr_buffer16;
//	  uint8_t  wr_buffer8_array[2];
//	};
};

void user_delay_ms(uint32_t period);
int8_t buf16Read(struct pcf8575_dev * dev, uint16_t * buf16);
int8_t pinRead (struct pcf8575_dev * dev, uint8_t pin);
int8_t pinWrite(struct pcf8575_dev * dev, uint8_t pin, uint8_t value);
void setPinMode(struct pcf8575_dev * dev, uint8_t pin, uint8_t mode);
struct pcf8575_dev * pcf8575_init(const char *iic_dev, const uint8_t iic_addr);
void pcf8575_close(struct pcf8575_dev *dev);

#endif /* PCF8575_H */
