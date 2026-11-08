#pragma once
// Portable capability and safety policy, shared unchanged with host regression.
#include <stddef.h>
#include <stdint.h>
struct Blaze8266Pin {
  uint8_t number;
  const char *label;
  bool exposed, input, output, adc, pwm, strap, reserved;
  const char *category, *warning;
};
enum Blaze8266Decision { BLAZE8266_DENY=0, BLAZE8266_ACK_REQUIRED=1, BLAZE8266_ALLOW=2 };
static const Blaze8266Pin BLAZE8266_PINS[] = {
  {0, "GPIO0 / D3", true,true,true,false,true,true,false,"strap","BOOT strap; LOW at reset enters flash download. External loads may prevent boot."},
  {1, "GPIO1 / TX", true,true,true,false,true,false,true,"uart","UART0 TX serial console; testing can interrupt diagnostics."},
  {2, "GPIO2 / D4", true,true,true,false,true,true,false,"strap","Boot strap must be HIGH at reset. Board LED may be active LOW."},
  {3, "GPIO3 / RX", true,true,true,false,true,false,true,"uart","UART0 RX serial console; testing may disrupt diagnostics."},
  {4, "GPIO4 / D2", true,true,true,false,true,false,false,"gpio","Common I2C SDA; disconnect active peripherals."},
  {5, "GPIO5 / D1", true,true,true,false,true,false,false,"gpio","Common I2C SCL; disconnect active peripherals."},
  {6, "GPIO6 / CLK", false,false,false,false,false,false,true,"flash","Connected to SPI flash. Always denied."},
  {7, "GPIO7 / SD0", false,false,false,false,false,false,true,"flash","Connected to SPI flash. Always denied."},
  {8, "GPIO8 / SD1", false,false,false,false,false,false,true,"flash","Connected to SPI flash. Always denied."},
  {9, "GPIO9 / SD2", false,false,false,false,false,false,true,"flash","Flash-dependent on ESP-12E/F; denied on reference boards."},
  {10,"GPIO10 / SD3",false,false,false,false,false,false,true,"flash","Flash-dependent on ESP-12E/F; denied on reference boards."},
  {11,"GPIO11 / CMD",false,false,false,false,false,false,true,"flash","Connected to SPI flash. Always denied."},
  {12,"GPIO12 / D6",true,true,true,false,true,false,false,"gpio","HSPI MISO; disconnect SPI peripherals."},
  {13,"GPIO13 / D7",true,true,true,false,true,false,false,"gpio","HSPI MOSI; disconnect SPI peripherals."},
  {14,"GPIO14 / D5",true,true,true,false,true,false,false,"gpio","HSPI SCLK; disconnect SPI peripherals."},
  {15,"GPIO15 / D8",true,true,true,false,true,true,false,"strap","Boot strap must be LOW at reset; loads can prevent startup."},
  {16,"GPIO16 / D0",true,true,true,false,false,false,false,"special","No conventional PWM/edge interrupts; internal pull-up not supported."},
  {17,"A0 / ADC",true,false,false,true,false,false,false,"analog","Analog input, not GPIO17; digital driving denied. Voltage limit depends on PCB divider."}
};
static const size_t BLAZE8266_PIN_COUNT = sizeof(BLAZE8266_PINS)/sizeof(BLAZE8266_PINS[0]);
inline const Blaze8266Pin *blaze8266Pin(int n) {
  return n >= 0 && static_cast<size_t>(n) < BLAZE8266_PIN_COUNT ? &BLAZE8266_PINS[n] : nullptr;
}
inline Blaze8266Decision blaze8266Policy(int n, bool output, bool ack) {
  const Blaze8266Pin *p = blaze8266Pin(n);
  if (!p || !p->exposed || (output ? !p->output : !p->input)) return BLAZE8266_DENY;
  if ((p->strap || p->reserved) && !ack) return BLAZE8266_ACK_REQUIRED;
  return BLAZE8266_ALLOW;
}


/* ESP8266 GPIO16 cannot be used for this software I2C scan. */
inline Blaze8266Decision blaze8266I2cPolicy(int sda, int scl, bool acknowledged) {
  if (sda == scl || sda == 16 || scl == 16) return BLAZE8266_DENY;
  const Blaze8266Pin *d = blaze8266Pin(sda), *c = blaze8266Pin(scl);
  if (!d || !c || !d->input || !c->input) return BLAZE8266_DENY;
  const Blaze8266Decision dp = blaze8266Policy(sda, true, acknowledged);
  const Blaze8266Decision cp = blaze8266Policy(scl, true, acknowledged);
  if (dp == BLAZE8266_DENY || cp == BLAZE8266_DENY) return BLAZE8266_DENY;
  if (dp == BLAZE8266_ACK_REQUIRED || cp == BLAZE8266_ACK_REQUIRED)
    return BLAZE8266_ACK_REQUIRED;
  return BLAZE8266_ALLOW;
}
