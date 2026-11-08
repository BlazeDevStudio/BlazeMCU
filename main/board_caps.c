#include "board_caps.h"

/* Reference profile: generic classic ESP32-WROOM-32 DevKit.
 * A chip capability is not proof that a particular manufacturer's PCB exposes it.
 * Every documented GPIO 0..39 is represented, including absent and flash pins.
 */
#define P(n, exp, in, out, adc_, pwm_, strap_, reserved_, cat, warn) \
  {n,exp,in,out,adc_,pwm_,strap_,reserved_,cat,warn}
static const blaze_pin_t pins[BLAZE_PIN_COUNT] = {
 P(0,1,1,1,1,1,1,0,"strap","BOOT strap. External loads may prevent startup."),
 P(1,1,1,1,0,1,0,1,"uart","UART0 TX/serial console; changing it may break diagnostics."),
 P(2,1,1,1,1,1,1,0,"strap","Bootstrap pin. LED presence depends on board."),
 P(3,1,1,1,0,1,0,1,"uart","UART0 RX/serial console; changing it may break diagnostics."),
 P(4,1,1,1,1,1,1,0,"strap","Strapping-sensitive GPIO; verify external wiring."),
 P(5,1,1,1,0,1,1,0,"strap","Bootstrap pin used by some SPI designs."),
 P(6,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(7,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(8,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(9,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(10,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(11,0,0,0,0,0,0,1,"flash","Internal SPI flash line. Selection only; never drive."),
 P(12,1,1,1,1,1,1,0,"strap","MTDI strap may select flash voltage; do not pull during boot."),
 P(13,1,1,1,1,1,0,0,"gpio","ADC2 unavailable for analog sampling while Wi-Fi operates."),
 P(14,1,1,1,1,1,0,0,"gpio","ADC2 unavailable for analog sampling while Wi-Fi operates."),
 P(15,1,1,1,1,1,1,0,"strap","MTDO strapping pin; boot output behavior may vary."),
 P(16,1,1,1,0,1,0,0,"gpio","WROOM-32 only; may be occupied by PSRAM on other modules."),
 P(17,1,1,1,0,1,0,0,"gpio","WROOM-32 only; may be occupied by PSRAM on other modules."),
 P(18,1,1,1,0,1,0,0,"gpio","Often SPI SCLK; release other peripherals before testing."),
 P(19,1,1,1,0,1,0,0,"gpio","Often SPI MISO; release other peripherals before testing."),
 P(20,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(21,1,1,1,0,1,0,0,"gpio","Often I2C SDA; do not share with an active bus."),
 P(22,1,1,1,0,1,0,0,"gpio","Often I2C SCL; do not share with an active bus."),
 P(23,1,1,1,0,1,0,0,"gpio","Often SPI MOSI; release other peripherals before testing."),
 P(24,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(25,1,1,1,1,1,0,0,"gpio","DAC/ADC2 functions exist; digital tests supported."),
 P(26,1,1,1,1,1,0,0,"gpio","DAC/ADC2 functions exist; digital tests supported."),
 P(27,1,1,1,1,1,0,0,"gpio","ADC2 unavailable for analog sampling while Wi-Fi operates."),
 P(28,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(29,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(30,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(31,0,0,0,0,0,0,0,"absent","Not a classic ESP32-WROOM-32 GPIO."),
 P(32,1,1,1,1,1,0,0,"gpio","ADC1 and digital GPIO available."),
 P(33,1,1,1,1,1,0,0,"gpio","ADC1 and digital GPIO available."),
 P(34,1,1,0,1,0,0,0,"input-only","Input only; no internal pull-up/down. Output unsupported."),
 P(35,1,1,0,1,0,0,0,"input-only","Input only; no internal pull-up/down. Output unsupported."),
 P(36,1,1,0,1,0,0,0,"input-only","Input only; no internal pull-up/down. Output unsupported."),
 P(37,0,0,0,1,0,0,0,"not-exposed","Input-capable silicon pad, not available on WROOM-32 module pins."),
 P(38,0,0,0,1,0,0,0,"not-exposed","Input-capable silicon pad, not available on WROOM-32 module pins."),
 P(39,1,1,0,1,0,0,0,"input-only","Input only; no internal pull-up/down. Output unsupported.")
};
#undef P

const blaze_pin_t *blaze_pin_get(int pin) {
  return (pin >= 0 && pin < BLAZE_PIN_COUNT) ? &pins[pin] : 0;
}
const char *blaze_board_id(void) { return "esp32-wroom-32-devkit-reference"; }
