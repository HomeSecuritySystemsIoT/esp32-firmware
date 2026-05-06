file(GLOB UTILITY_SRCS "utilities/*.c")

idf_component_register(SRCS "main.c" "station_wifi.c"  "soft_ap_sub.c" "camera.c" "server_reacher.c" "jpeg.cpp" "http_website.c" "JPEGDEC/JPEGDEC.cpp" "wifi_common.c" ${UTILITY_SRCS}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs_com/esp32_cert.crt"
                                   "certs_com/esp32_private.key"
                                   "certs_com/ca_cert.crt"
                    )
