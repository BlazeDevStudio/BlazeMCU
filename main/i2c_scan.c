#include "i2c_scan.h"
#include "lab.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/* The scan only probes seven-bit address ACK, without writes to registers. */
esp_err_t blaze_i2c_scan(int sda, int scl, bool ack,
                         uint8_t *addresses, size_t capacity, size_t *found) {
  if (!addresses || !found || capacity < BLAZE_I2C_MAX_RESPONDERS)
    return ESP_ERR_INVALID_ARG;
  *found = 0;
  if (blaze_i2c_decide(sda, scl, ack) != BLAZE_ALLOW)
    return ESP_ERR_NOT_SUPPORTED;
  /* The HTTP server handles GPIO mutations serially; refuse conflicting
   * laboratory operation rather than implicitly stopping any active test.
   */
  if (blaze_lab_status().pin >= 0) return ESP_ERR_INVALID_STATE;
  i2c_master_bus_config_t config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = (gpio_num_t)scl,
    .sda_io_num = (gpio_num_t)sda,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true
  };
  i2c_master_bus_handle_t bus = NULL;
  esp_err_t result = i2c_new_master_bus(&config, &bus);
  if (result != ESP_OK) return result;
  const int64_t deadline = esp_timer_get_time() + 1500000LL;
  for (int address = 0x08; address <= 0x77; ++address) {
    if (esp_timer_get_time() >= deadline) { result = ESP_ERR_TIMEOUT; break; }
    const esp_err_t probe = i2c_master_probe(bus, address, 5);
    if (probe == ESP_OK) addresses[(*found)++] = (uint8_t)address;
    else if (probe != ESP_ERR_NOT_FOUND) { result = probe; break; }
  }
  const esp_err_t cleanup = i2c_del_master_bus(bus);
  gpio_reset_pin((gpio_num_t)sda);
  gpio_reset_pin((gpio_num_t)scl);
  if (result == ESP_OK) result = cleanup;
  if (result != ESP_OK) *found = 0;
  return result;
}
