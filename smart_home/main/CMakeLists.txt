idf_component_register(SRCS "smart_home.c"
                    INCLUDE_DIRS "."
                     PRIV_REQUIRES esp_wifi esp_event nvs_flash esp_netif esp_http_server esp_driver_gpio)
