#include <stdio.h>
#include <string.h>
#include "esp_https_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"

extern const unsigned char server_crt_start[] asm("_binary_server_crt_start");
extern const unsigned char server_crt_end[]   asm("_binary_server_crt_end");

extern const unsigned char server_key_start[] asm("_binary_server_key_start");
extern const unsigned char server_key_end[]   asm("_binary_server_key_end");
#define RELAY1 GPIO_NUM_16
#define RELAY2 GPIO_NUM_17

#define WIFI_SSID "Airtel_moha_1798"
#define WIFI_PASS "Air@92605"

static const char *TAG = "SMART_HOME";
bool relay1_state = false;
bool relay2_state = false;
/* ---------------- WEB PAGE ---------------- */



/* ---------------- HTTP HANDLERS ---------------- */

static esp_err_t root_handler(httpd_req_t *req)
{
    static char html[8000];

    sprintf(html,
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
       //removed <meta http-equiv='refresh' content='5'>
        "<title>Smart Home</title>"

        "<style>"
        "body{background:#1b1b1b;color:white;font-family:Arial;text-align:center;}"
        ".card{background:#2d2d2d;margin:20px auto;padding:20px;border-radius:15px;width:300px;box-shadow:0 0 10px #000;}"
        "button{width:120px;height:50px;font-size:20px;border:none;border-radius:10px;margin:10px;color:white;cursor:pointer;}"
        ".on{background:#28a745;}"
        ".off{background:#dc3545;}"
        ".statuson{color:#00ff66;font-size:22px;font-weight:bold;}"
        ".statusoff{color:#ff4444;font-size:22px;font-weight:bold;}"
        "</style>"

        "</head>"
       

        
        "<body>"

        "<h1>🏠 Smart Home</h1>"

        "<div class='card'>"
        "<h2>💡 Bedroom Light</h2>"
        "<p>Status : <span class='%s'>%s</span></p>"
        "<a href='/relay1on'><button class='on'>ON</button></a>"
        "<a href='/relay1off'><button class='off'>OFF</button></a>"
        "</div>"

        "<div class='card'>"
        "<h2>🌀 Fan</h2>"
        "<p>Status : <span class='%s'>%s</span></p>"
        "<a href='/relay2on'><button class='on'>ON</button></a>"
        "<a href='/relay2off'><button class='off'>OFF</button></a>"
        "</div>"

        "<h3>ESP32 Smart Home Project</h3>"
        "<hr>"

          "<button onclick='startVoice()' "
          "style='width:220px;height:60px;"
          "font-size:22px;"
          "background:#007BFF;"
          "color:white;"
          "border:none;"
          "border-radius:12px;'>"

          "Voice Control"

        "</button>"
          "<script>"

"function startVoice(){"

    "const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;"

    "if (!SpeechRecognition){"
        "alert('Speech Recognition not supported in this browser.');"
        "return;"
    "}"

    "const recognition = new SpeechRecognition();"

    "recognition.lang = 'en-US';"
   "recognition.interimResults = false;"
    "recognition.maxAlternatives = 1;"

    "recognition.onstart = function (){"
        "console.log('Listening...');"
    "};"

    "recognition.onresult = function (event){"

        "let command = event.results[0][0].transcript.toLowerCase();"

        "alert('You said: ' + command);"

        "if (command.includes('turn on light')){"
           "window.location.href = '/relay1on';"
       "}"
        "else if (command.includes('turn off light')){"
            "window.location.href = '/relay1off';"
        "}"
        "else if (command.includes('turn on fan')){"
            "window.location.href = '/relay2on';"
        "}"
        "else if (command.includes('turn off fan')){"
            "window.location.href = '/relay2off';"
        "}"
        "else {"
            "alert('Command not recognized.');"
        "}"
    "};"

    "recognition.onerror = function (event){"
        "alert('Error: ' + event.error);"
    "};"

    "recognition.onend = function (){"
        "console.log('Voice recognition ended');"
    "};"

    "recognition.start();"
"}"

"</script>"
        "</body>"
        "</html>",

        relay1_state ? "statuson" : "statusoff",
        relay1_state ? "ON" : "OFF",

        relay2_state ? "statuson" : "statusoff",
        relay2_state ? "ON" : "OFF"
    );

    httpd_resp_set_type(req,"text/html; charset=UTF-8" );
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t relay1_on(httpd_req_t *req)
{
    gpio_set_level(RELAY1, 0);
    relay1_state = true;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t relay1_off(httpd_req_t *req)
{
    gpio_set_level(RELAY1, 1);
    relay1_state = false;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t relay2_on(httpd_req_t *req)
{
    gpio_set_level(RELAY2, 0);
    relay2_state = true;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t relay2_off(httpd_req_t *req)
{
    gpio_set_level(RELAY2, 1);
    relay2_state = false;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* ---------------- WEB SERVER ---------------- */
static httpd_handle_t start_webserver(void)
{
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    conf.servercert = server_crt_start;
    conf.servercert_len = server_crt_end - server_crt_start;
    
    conf.prvtkey_pem = server_key_start;
    conf.prvtkey_len = server_key_end - server_key_start;

    httpd_handle_t server = NULL;

    if (httpd_ssl_start(&server, &conf) == ESP_OK)
    {
        httpd_uri_t root = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_handler,
            .user_ctx = NULL
        };

        httpd_uri_t r1on = {
            .uri = "/relay1on",
            .method = HTTP_GET,
            .handler = relay1_on,
            .user_ctx = NULL
        };

        httpd_uri_t r1off = {
            .uri = "/relay1off",
            .method = HTTP_GET,
            .handler = relay1_off,
            .user_ctx = NULL
        };

        httpd_uri_t r2on = {
            .uri = "/relay2on",
            .method = HTTP_GET,
            .handler = relay2_on,
            .user_ctx = NULL
        };

        httpd_uri_t r2off = {
            .uri = "/relay2off",
            .method = HTTP_GET,
            .handler = relay2_off,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &r1on);
        httpd_register_uri_handler(server, &r1off);
        httpd_register_uri_handler(server, &r2on);
        httpd_register_uri_handler(server, &r2off);

        ESP_LOGI(TAG, "HTTPS Server Started");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start HTTPS server");
    }

    return server;
}

/* ---------------- WIFI ---------------- */
static void wifi_init_sta(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi...");

    ESP_ERROR_CHECK(esp_wifi_connect());
}

/* ---------------- MAIN ---------------- */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Relay initialization
    gpio_reset_pin(RELAY1);
    gpio_set_direction(RELAY1, GPIO_MODE_OUTPUT);

    gpio_reset_pin(RELAY2);
    gpio_set_direction(RELAY2, GPIO_MODE_OUTPUT);

    // Relay OFF initially (active LOW)
    gpio_set_level(RELAY1, 1);
    gpio_set_level(RELAY2, 1);

    wifi_init_softap();
    start_webserver();

    ESP_LOGI(TAG, "Web Server Started");
}