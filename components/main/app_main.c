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

    switch(result.type) {
        case SUCCESS: {
            if (result.return_value.type == VAL_NUMBER) {
                printf("return val: %d\n", result.return_value.as.number);
            }
            break;
        }
        // TODO: エラー対応．ホスト側も合わせて要検討
        case ERROR_DIVISION_BY_ZERO: {
            printf("error\n");
            break;
        }
        case ERROR_UNKNOWN_OPCODE: {
            printf("error\n");
            break;
        }
        case ERROR_NO_METHOD: {
            break;
        }
        case ERROR_OTHER: {
            break;
        }
    }

    // restart
    free(data.content);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
