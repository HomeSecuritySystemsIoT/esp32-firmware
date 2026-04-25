idf_component_register(SRCS "softap_example_main.c"
							"station_wifi.c"
							"camera.c"
							"led.c"
							"http_website.c" INCLUDE_DIRS
							"." EMBED_TXTFILES "certs/servercert.pem"
							"certs/prvtkey.pem")
