#include <stdio.h>
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "esp_sleep.h" 
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void);
//static void update_pulse_count(void);

void app_main(void)
{
    printf("Starting main proc...\n");
    printf("Pulse Count = %d \n", ulp_pulse_count & UINT16_MAX);
    printf("Last reading = %d \n", ulp_last_reading & UINT16_MAX);
    printf("Last result = %d \n", ulp_last_result & UINT16_MAX);

    printf("Starting ULP...\n");
    init_ulp_program();

    const int wakeup_time_sec = 5;
    printf("Enabling timer wakeup for main proc \n");
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    printf("Going to sleep..\n");
    fflush(stdout);
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}

static void init_ulp_program(void)
{
    // Load ulp binary from the image
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start)/sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    // Disconnect GPIO12 and GPIO15 to remove current drain through
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages

    // Configure ADC channel
    // Note: when changing channel here, also change 'adc_channel' constant in pulse_counter.S
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();

    // Set low and high thresholds, as per the readings from 49E hall sensor from water meter
    ulp_low_threshold = 1600;
    ulp_high_threshold = 1800;

    // Run ulp program every 20ms
    ulp_set_wakeup_period(0, 20000);

    // Start the program
    err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
