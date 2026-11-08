#include "oled_diag.h"
#include "i2c_policy.h"
#include "lab.h"
#include "../common/oled_pattern.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/* Display-driver operation is explicit, never inferred from a scanned address.
 * Only 0x3C/0x3D; one 128x64 display and one limited 128-byte row at a time.
 */
esp_err_t blaze_oled_draw(int sda, int scl, int address, int controller,
                          int pattern, bool acknowledged) {
  if (!blaze_oled_allowed(address,controller,pattern)) return ESP_ERR_INVALID_ARG;
  if (blaze_i2c_decide(sda,scl,acknowledged) != BLAZE_ALLOW)
    return ESP_ERR_NOT_SUPPORTED;
  if (blaze_lab_status().pin >= 0) return ESP_ERR_INVALID_STATE;

  i2c_master_bus_config_t cfg = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = (gpio_num_t)scl,
    .sda_io_num = (gpio_num_t)sda,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true
  };
  i2c_master_bus_handle_t bus = NULL;
  i2c_master_dev_handle_t device = NULL;
  esp_err_t result = i2c_new_master_bus(&cfg,&bus);
  if (result != ESP_OK) return result;
  result = i2c_master_probe(bus,address,5);
  if (result != ESP_OK) goto cleanup;
  i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = (uint16_t)address,
    .scl_speed_hz = 100000
  };
  result = i2c_master_bus_add_device(bus,&dev_cfg,&device);
  if (result != ESP_OK) goto cleanup;
  const int64_t deadline = esp_timer_get_time() + 2000000LL;
  size_t command_len = 0;
  const uint8_t *commands = blaze_oled_init_commands(controller,&command_len);
  result = i2c_master_transmit(device,commands,command_len,20);
  if (result != ESP_OK) goto cleanup;
  for (unsigned page = 0; page < BLAZE_OLED_PAGES && result == ESP_OK; ++page) {
    uint8_t cmd[4];
    blaze_oled_page_commands(controller,page,cmd);
    result = i2c_master_transmit(device,cmd,sizeof(cmd),20);
    for (unsigned col = 0; col < BLAZE_OLED_WIDTH && result == ESP_OK;
         col += BLAZE_OLED_CHUNK) {
      if (esp_timer_get_time() > deadline) { result = ESP_ERR_TIMEOUT; break; }
      uint8_t packet[BLAZE_OLED_CHUNK + 1];
      blaze_oled_data_chunk(pattern,page,col,packet);
      result = i2c_master_transmit(device,packet,sizeof(packet),20);
    }
  }
cleanup:
  if (device) {
    esp_err_t e = i2c_master_bus_rm_device(device);
    if (result == ESP_OK) result = e;
  }
  if (bus) {
    esp_err_t e = i2c_del_master_bus(bus);
    if (result == ESP_OK) result = e;
    gpio_reset_pin((gpio_num_t)sda);
    gpio_reset_pin((gpio_num_t)scl);
  }
  return result;
}
