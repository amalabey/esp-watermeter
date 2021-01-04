#include "soc/soc_ulp.h"     // for WRITE_RTC_REG
#include "soc/rtc_io_reg.h"  // for RTC_GPIO_*

/* constants */
.set adc_channel, 0 // ADC Channel 0, GPIO36
.set adc_oversampling_factor_log, 2
.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)

/* variable declrarations */
.bss

.global low_threshold
low_threshold:
    .long 0

.global high_threshold
high_threshold:
    .long 0


.global last_result
last_result:
    .long 0

.global pulse_count
pulse_count:
    .long 0

/* code */
.text
.global entry
entry:
    // Load last result (1 = HIGH, 0 = LOW) into r2 from RTC memory
    move r3, last_result
    ld r2, r3, 0

measure:
    // Measure value and accumulate in r0
    adc r1, 0, adc_channel + 1
    add r0, r0, r1
    stage_inc 1
    jumps measure, adc_oversampling_factor, lt

    // Get the average by dividing by oversampling factor
    rsh r0, r0, adc_oversampling_factor_log

    // Compare with low threshold
    move r3, low_threshold
    ld r3, r3, 0
    sub r3, r0, r3
    jump low_edge, ov

    // Compare with high threshold
    move r3, high_threshold
    ld r3, r3, 0
    sub r3, r3, r0
    jump high_edge, ov

    halt

low_edge:
    // Check previous result, if it was high
    move r0, 0 // Set current result as LOW
    move r1, last_result
    ld r1, r1, 0
    sub r1, r0, r1
    jump cycle_complete, ov

    halt

high_edge:
    // Check previous result, if it was high
    move r0, 1 // Set current result as HIGH
    move r1, last_result
    ld r1, r1, 0
    sub r1, r1, r0
    jump cycle_complete, ov

    halt

cycle_complete:
    // Store result in last_result
    move r1, last_result
    st r0, r1, 0

    // Increse pulse count
    move r1, pulse_count
    ld r2, r1, 0
    add r2, r1, 1
    st r2, r1, 0

    halt
