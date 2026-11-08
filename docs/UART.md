# BlazeMCU UART RX monitor — 0.6 preview

Only **receive** operations are supported in this first bounded implementation.
UART TX/transmit, hardware flow control, 5 V TTL and RS-232 serial levels are
never driven. ESP GPIO logic is 3.3 V. A separate serial interface must share
ground with the ESP and be voltage-compatible; use approved level shifting
where required. RS-232 requires a proper MAX3232-like transceiver, never a
direct connection.

The UART0 provisioning/debug serial console pins GPIO1 and GPIO3 cannot be
taken over. ESP32 uses peripheral UART2 to monitor a user-selected RX GPIO;
on the ESP32 input-only pins GPIO34/35/36/39 can receive but have no internal
pull-ups. ESP8266 uses the EspSoftwareSerial library bundled in the pinned
ESP8266 Arduino board framework. Its GPIO16 has no conventional input edge
interrupt; flash pins GPIO6–11, the analog-only A0, and UART0 GPIO1/3 are denied.

The monitor accepts exactly 1200, 2400, 4800 or 9600 baud, 8N1.
One session lasts at most 20 seconds and must be rearmed with operator action
to continue. HTTP polling drains at most 128 bytes per request as hexadecimal
plus a sanitized ASCII preview. Incoming data is not persisted, and the
browser truncates its log instead of accumulating unbounded text. Transmitted
data cannot be initiated from the browser.

UART sessions cannot overlap GPIO lab commands, I²C scans or OLED writes.
The RX pin returns to input after STOP, timeout or normal driver shutdown.
This is software-level best-effort cleanup, not electrical proof or a
guarantee about attached peripherals, radio-interference or interrupt load.
Physical end-to-end input, framing accuracy, baud tolerance, GPIO safe idle,
power and watchdog behavior need real ESP32/ESP8266 acceptance tests.

Any successful browser mock test proves only frontend request/response behavior,
not that UART data was received from actual hardware.
