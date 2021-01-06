#include <stdio.h>
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "esp_sleep.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ulp_main.h"
#include "data_logger.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void init_ulp_program(void);
static void start_ulp_program(void);
static void update_pulse_count(void);

#ifdef ULP_DEBUG
static void dump_debug_out(void);
#endif

RTC_DATA_ATTR int total_pulse_count = 0;

void app_main(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_ULP) {
        printf("ULP wakeup\n");
        update_pulse_count();
    } else if(cause == ESP_SLEEP_WAKEUP_TIMER ) {
        printf("Timer wake up\n");
        update_pulse_count();
    } else  {
        printf("Not ULP wakeup\n");
        init_ulp_program();
    }

    start_ulp_program();
    
#ifdef ULP_DEBUG
    dump_debug_out();
#endif

    const int wakeup_time_sec = 60;
    printf("Enabling timer wakeup for main proc \n");
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    printf("Going to sleep..\n");
    fflush(stdout);
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
    esp_deep_sleep_start();
}

#ifdef ULP_DEBUG
static void dump_debug_out()
{
    while (true)
    {
        printf("Pulse Count = %d", ulp_pulse_count & UINT16_MAX);
        printf(", Last reading = %d", ulp_debug_data & UINT16_MAX);
        printf(", Last result = %d \n", ulp_last_result & UINT16_MAX);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
#endif

static void update_pulse_count()
{
    printf("Pulse Count = %d", ulp_pulse_count & UINT16_MAX);
    printf(", Last result = %d", ulp_last_result & UINT16_MAX);
    printf(", debug data = %d \n", ulp_debug_data & UINT16_MAX);

    total_pulse_count += ulp_pulse_count & UINT16_MAX;
    printf("Total pulse count: %d \n", total_pulse_count);

    const sensor_data_t data = {
        .water_consumption = total_pulse_count
    };
    log_data(&data);

    ulp_pulse_count = 0;
}

static void init_ulp_program(void)
{
    printf("Loading ULP program.\n");
    // Load ulp binary from the image
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start)/sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    // Configure ADC channel
    // Note: when changing channel here, also change 'adc_channel' constant in pulse_counter.S
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();

    // Set constant thresholds
    ulp_low_threshold = 1600;
    ulp_high_threshold = 1800;
    ulp_wakeup_threshold = 3;

    // Run ulp program every 20ms
    ulp_set_wakeup_period(0, 20000);  

    // Disconnect GPIO12 and GPIO15 to remove current drain through
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);
    esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

static void start_ulp_program(void)
{
    printf("Starting ULP program.\n");
    
    // Initialize variables
    ulp_pulse_count = 0;

    // Start the program
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
