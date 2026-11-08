idf_component_register(
  SRCS "app_main.c" "board_caps.c" "lab_policy.c" "lab.c" "security.c" "api.c" "i2c_policy.c" "i2c_scan.c"
  INCLUDE_DIRS "."
  EMBED_TXTFILES "web/index.html" "web/style.css" "web/app.js"
  REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_http_server json driver esp_timer spi_flash bootloader_support esp_hw_support esp_driver_ledc esp_driver_i2c
)
