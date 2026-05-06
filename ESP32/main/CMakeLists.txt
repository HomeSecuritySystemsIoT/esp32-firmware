file(GLOB UTILITY_SRCS "utilities/*.c")
file(GLOB NETWORKING_SRCS "networking/*.c")
file(GLOB PERIPHERALS_SRC "peripherals/*.c")

idf_component_register(SRCS "main.c" "jpeg.cpp" "JPEGDEC/JPEGDEC.cpp" ${UTILITY_SRCS} ${NETWORKING_SRCS} ${PERIPHERALS_SRC}
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES "certs_com/esp32_cert.crt"
                                   "certs_com/esp32_private.key"
                                   "certs_com/ca_cert.crt"
                    )
