idf_component_register(
    SRCS "smart_home.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES
        "../certs/server.crt"
        "../certs/server.key"
    PRIV_REQUIRES
        esp_https_server
        esp_http_server
        esp_wifi
        esp_event
        nvs_flash
        esp_netif
        esp_driver_gpio
)