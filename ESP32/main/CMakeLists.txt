idf_component_register(SRCS "softap_example_main.c" "station_wifi.c" "camera.c" "calc_diff.S" "jpeg.cpp" "led.c" "http_website.c" "JPEGDEC/JPEGDEC.cpp" 
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs/servercert.pem"
                                   "certs/prvtkey.pem"          
                    )
                    





                    