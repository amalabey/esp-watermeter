#include <stdio.h>
#include "esp32/ulp.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void);
//static void update_pulse_count(void);

void app_main(void)
{
    init_ulp_program();
}

static void init_ulp_program(void)
{
    // Load ulp binary from the image
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start)/sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    // Run ulp program every 20ms
    ulp_set_wakeup_period(0, 20000);

    // Start the program
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
