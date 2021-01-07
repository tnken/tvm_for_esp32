#include <stdlib.h>

typedef struct {
  uint16_t size;
  uint8_t* content;
} InputData;

InputData read_data_from_usb_serial();
