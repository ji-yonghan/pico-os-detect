/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Ji-yong Han
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * This software makes use of the following software
 * 
 * TinyUSB Library Copyright (c) 2019 Ha Thach (tinyusb.org) (The MIT License (MIT))
 * Raspberry Pi RP2040 SDK Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd. (BSD 3-Clause "New" or "Revised" License)
 */

//--------------------------------------------------------------------+
// Inclues
//--------------------------------------------------------------------+
/*------------- Standard C Libs -------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*------------- TinyUSB -------------*/
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

/*------------- UART -------------*/
#include "pico/stdlib.h"
#include "hardware/uart.h"

//--------------------------------------------------------------------+
// UART Config
//--------------------------------------------------------------------+
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

//--------------------------------------------------------------------+
// Modes
//--------------------------------------------------------------------+
uint8_t guessed = 0; // Used as a means of knowing the state of the OS detection. 0 is not yet complete, 1 = an OS was fingerprinted, 2 = Tried to fingerprint but failed.
uint8_t num_lock = 0; // This is used as a part of OS detection, Chrome OS devices can't set the LED when triggered. 

//--------------------------------------------------------------------+
// Constant Typefef Prototypes
//--------------------------------------------------------------------+
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

//--------------------------------------------------------------------+
// Function Prototypes
//--------------------------------------------------------------------+
void led_blinking_task(void);
void wake_task(void);
void guess_os(void);
void process_wlength(uint8_t desc_index, uint16_t wIndex, uint16_t wLength);

//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+
int main(void) {
  stdio_init_all(); // Init all of the GPIO

  // Setup UART
  uart_init(UART_ID, BAUD_RATE); 
  gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

  board_init();
  
  // Init TinyUSB
  tusb_init(); 

  while (1) {
    tud_task(); // tinyusb device task
    led_blinking_task(); // Update the blink pattern to reflect the device state

    wake_task(); // Wake up if possible
    guess_os(); // When ready, see if the fingerprint matches something we know   
  }
  return 0; // We shouldn't ever get here unless something went very wrong!
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+
// Invoked when device is mounted
void tud_mount_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  blink_interval_ms = BLINK_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
static void send_hid_report(uint8_t modifier, uint8_t key) {
  uint8_t keycode[6] = { 0 };
  keycode[0] = key;

  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
}

void wake_task(void) {
  // Try and wake up host if we are suspended
  if ( tud_suspended() ) {
    tud_remote_wakeup();
  }
}

// Invoked when sent REPORT successfully to host
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
  (void) instance;
  (void) len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT) {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD) {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_NUMLOCK) {
        num_lock = 1;
        printf("Numlock called - Flag State = %u\r\n", num_lock);
      } 
    }
  }
}

//--------------------------------------------------------------------+
// wLength Processing / OS Detection
//--------------------------------------------------------------------+
struct setups_data_t {
    uint8_t count;
    uint8_t count_402;
    uint8_t count_20A;
    uint8_t count_2;
    uint8_t count_4;
    uint8_t count_E;
    uint8_t count_1E;
    uint8_t count_10;
    uint8_t count_ff;
    uint8_t count_fe;
    uint16_t last_wlength;
};

struct setups_data_t setups_data = {
    .count = 0,
    .count_402 = 0,
    .count_20A = 0,
    .count_2 = 0,
    .count_4 = 0,
    .count_E = 0,
    .count_1E = 0,
    .count_10 = 0,
    .count_ff = 0,
    .count_fe = 0,
};

// This is called from usb_descripters.c
void process_wlength(uint8_t desc_index, uint16_t wIndex, uint16_t wLength){
  setups_data.count++;
  setups_data.last_wlength = wLength;

  if (wLength == 0x402) {
    setups_data.count_402++;
  } else if (wLength == 0x20A) {
    setups_data.count_20A++;
  } else if (wLength == 0x2) {
    setups_data.count_2++;
  } else if (wLength == 0x4) {
    setups_data.count_4++;
  } else if (wLength == 0xE) {
    setups_data.count_E++;
  } else if (wLength == 0x1E) {
    setups_data.count_1E++;
  } else if (wLength == 0x10) {
    setups_data.count_10++;
  } else if (wLength == 0xFF) {
    setups_data.count_ff++;
  } else if (wLength == 0xFE) {
    setups_data.count_fe++;
  }

  printf("DescriptorIndex -> %X, wIndex -> %X, wLength -> %X\r\n", desc_index, wIndex, wLength);
}

void guess_os() {
  const uint32_t interval_ms = 2000; // This determins how long we wait for the USB initalisation to complete before trying to guess the OS
  static uint32_t start_ms = 0;

  // We need to wait enough time before we try and detect the OS
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  // if guessed is not yet set to 1 (Finger print matched) or 2 (OS unknown) then we look to test, or we just call it quits. 
  if (guessed == 0 && guessed != 2) {
    // There should be at least 3 requests becase there should be a brand model and serial request made
    if (setups_data.count < 3) {
      printf("OS_UNSURE, Not enough headers\r\n");
      guessed = 2;
    }

    // Linux Kernels seem to only make 5 FF requests, However ChromeOS uses the same kernel and can't taggle the numlock, so we should test this here.
    if (setups_data.count == 5 && setups_data.count_ff == 5) { 
      send_hid_report(0, HID_KEY_NUM_LOCK);
      sleep_ms(255);
      send_hid_report(0, HID_KEY_NUM_LOCK);
      sleep_ms(255);
      send_hid_report(0, HID_KEY_NUM_LOCK);
      sleep_ms(255);
      send_hid_report(0, HID_KEY_NUM_LOCK);
      sleep_ms(255);
      if (num_lock == 1) {
        printf("I think the OS is Linux!\r\n");
        guessed = 1;
      } else {
        printf("I think the OS is ChromeOS!\r\n");
        guessed = 1;
      }
    }

    // FreeBSD seems to be the only OS that has length counts of 2 and 4 across 8 requests
    if (setups_data.count == 8 && setups_data.count_2 == 4 && setups_data.count_E == 1 &&  setups_data.count_1E == 1) {
      printf("I think the OS is FreeBSD\r\n");
      guessed = 1;
    }

    // Some windows machines only seem to make three FF requests
    if (setups_data.count == 3 && setups_data.count_ff == 3) {
      printf("I think the OS is Windows, But the version that only has three packets for setup\r\n");
      guessed = 1;
    }
    
    // Some windows machines only make 6 FF requests
    if (setups_data.count == 6 && setups_data.count_ff == 6) {
      printf("I think the OS is Windows, But the version that has six packets for setup\r\n");
      guessed = 1;
    }

    // Other windows machines seem to make more than 10 requests when they are getting setup, with a good number been FF
    if (setups_data.count > 10 && setups_data.count_ff > 10) {
      printf("I think this is a Windows machine, but the version that makes lots of requests\r\n");
      guessed = 1;
    }

    // Android seems to consistantly make 17 calls 11 FF's and 6 FEs and is the only one to send "FE" packets
    if (setups_data.count == 17 && setups_data.count_ff == 11 && setups_data.count_fe == 6) {
      printf("I think the OS is Android\r\n");
      guessed = 1;
    }

    // Intel MacOS Sends 2, 1E, 2, 10, 2, E and doesn't support numlock
    if (setups_data.count == 6 && setups_data.count_2 == 3 && setups_data.count_E == 1) {
      printf("I think the host OS is MacOS (Intel)\r\n");
      guessed = 1;
    }

    // ARM MacOS Sends 2, 1E, 2, 10, 2, E, FF and doesn't support numlock
    if (setups_data.count == 7 && setups_data.count_2 == 3 && setups_data.count_E == 1 && setups_data.count_ff == 1) {
      printf("I think the host OS is MacOS (ARM)\r\n");
      guessed = 1;
    }
    
    // iOS Sends 2, 1E, 2, 10, 2, E and doesn't support numlock, because this is the same as Intel MacOS devices, I am disabling this for now
    //if (setups_data.count == 6 && setups_data.count_2 == 3 && setups_data.count_E == 1) {
    //  printf("I think the host os is iOS\r\n");
    // guessed = 1;
    //}

    // Check to see if we are still at guessed == 0 and call it quits as we don't know what is on the other end
    if (guessed == 0) {
      printf("I don't know what the OS of the host as I as I don't know the fingerprint\r\n");
      guessed = 2;
    }
  }
}


//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
