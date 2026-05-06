file(GLOB UTILITY_SRCS "utilities/*.c")
file(GLOB NETWORKING_SRCS "networking/*.c")

idf_component_register(SRCS "main.c" "camera.c" "jpeg.cpp" "JPEGDEC/JPEGDEC.cpp" ${UTILITY_SRCS} ${NETWORKING_SRCS}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs_com/esp32_cert.crt"
                                   "certs_com/esp32_private.key"
                                   "certs_com/ca_cert.crt"
                    )
