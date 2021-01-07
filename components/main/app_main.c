#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "peripheral.h"
#include "vm.h"

void app_main(void)
{
    InputData data = read_data_from_usb_serial();
    ExecResult result = tarto_vm_run((char*) data.content);
    if (result.type == SUCCESS) {
        printf("return val: %d\n", result.return_value);
    }

    // restart
    free(data.content);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
