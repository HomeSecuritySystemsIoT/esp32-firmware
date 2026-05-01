idf_component_register(SRCS "main.c" "station_wifi.c"  "soft_ap_sub.c""camera.c" "server_reacher.c" "jpeg.cpp" "led.c" "http_website.c" "JPEGDEC/JPEGDEC.cpp" "wifi_common.c" 
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs/servercert.pem"
                                   "certs/prvtkey.pem"          
                    )
                    





                    