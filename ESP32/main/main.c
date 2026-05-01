/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "includes.h"
#include"station_wifi.h"
#include"soft_ap_sub.h"



/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/





void app_main(void)
{
	//nvs_flash_erase();
	esp_err_t ret = nvs_flash_init();
   	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
   		ESP_ERROR_CHECK(nvs_flash_erase());
    	ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    
    esp_netif_set_hostname(wifi_netif, "myesp32.xws");

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
	esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 1,
    .format_if_mount_failed = true
	};
	ret=esp_vfs_spiffs_register(&conf);
	//goto huj;
	//if(!remove("/spiffs/wf")){
	//	puts("removed");
	//}
	if(ret!=ESP_OK){
		printf("esp_vfs_spiffs_register(): %u\n",ret);
	}
	
	esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
	cfg.stack_size = 16384;        
	cfg.prio = 5;
	cfg.inherit_cfg = false;      
	esp_pthread_set_cfg(&cfg);
	
	
	pthread_t th1;
	pthread_attr_t attr_detached;
	pthread_attr_init(&attr_detached);
	pthread_attr_setdetachstate(&attr_detached, PTHREAD_CREATE_DETACHED);
	
	pthread_attr_setstacksize(&attr_detached, 16384);
	
	puts("esp_vfs_spiffs_register(): OK");
	FILE * wififile=fopen("/spiffs/wf","rb");
	wifi_name.data=(char*)malloc(98*UTF8_MAX_SIZE);
	if(!wifi_name.data)
			exit(556);
	wifi_password.data=wifi_name.data+33*UTF8_MAX_SIZE;
	wifi_name.size=0;
	wifi_password.size=0;
	int wifi_getter=1;
	configure_led();
	if(wififile){
		puts("Found cache file");
		fread(&wifi_name.size,1,1,wififile);
		fread(&wifi_password.size,1,1,wififile);
		fread(wifi_name.data,wifi_name.size,1,wififile);
		fread(wifi_password.data,wifi_password.size,1,wififile);
		fclose(wififile);
		wifi_name.data[wifi_name.size]=0;
		wifi_password.data[wifi_password.size]=0;
		wifi_getter=wifi_init_sta(&wifi_name,&wifi_password);
		if(wifi_getter){
			vTaskDelay(pdMS_TO_TICKS(5000));
			wifi_getter=wifi_init_sta(&wifi_name,&wifi_password);
		}
	}
	httpd_handle_t server = NULL;
    //Initialize NVS
    
	while(wifi_getter){
		has_wifi=0;
    	wifi_init_softap();
    	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server));
    	while(!has_wifi){
			blink_led();
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		stop_webserver(*SERVER);
		esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler);

		wifi_destroy();
		*SERVER=NULL;
		
		vTaskDelay(pdMS_TO_TICKS(5000));
    	printf("Attempting to find : %s with password %s\n",wifi_name.data,wifi_password.data);
    	wifi_getter=wifi_init_sta(&wifi_name,&wifi_password);
    	if(wifi_getter){
			vTaskDelay(pdMS_TO_TICKS(16000));
		}
    	else{
			wififile=fopen("/spiffs/wf","wb");
			if(!wififile){
				exit(557);
			}
			esp_wifi_set_ps(WIFI_PS_NONE);
			fwrite(&wifi_name.size,1,1,wififile);
			fwrite(&wifi_password.size,1,1,wififile);
			fwrite(wifi_name.data,wifi_name.size,1,wififile);
			fwrite(wifi_password.data,wifi_password.size,1,wififile);
			fclose(wififile);
			puts("Saved to temp file");
		}
		
	}	
	free(wifi_name.data);
	set_led(0,0,0);
	//huj:
	//wifi_init_softap();
	
	time_t start,end;
	sleep(10);
	
	char receive_buff[1025];
	int err;
	
	//socket(AF_INET,SOCK_DGRAM,ip_protocol) socket(AF_INET,SOCK_STREAM,ip_protocol)
	int client_fd_udp=udp_connect_server("51.210.107.234","7892"),    
	client_fd_tcp=tcp_connect_server("51.210.107.234","7891");
	receive_buff[1024]=0;
		
	if (client_fd_tcp == -1) {
    	puts("client_fd_tcp=-1");
    	return;
	}
	if (client_fd_udp == -1) {
    	puts("client_fd_udp=-1");
    	return;
	}
	
	
	//struct timeval timeout;
	//timeout.tv_sec=10;
	//timeout.tv_usec=0;
	//setsockopt(client_fd_udp,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
	/*
	const char*tcp_message="Message TCP de l'esp32";
	size_t tcp_message_len=sizeof("Message TCP de l'esp32")-1;
	
	
	const char*udp_message="Message UDP de l'esp32";
	size_t udp_message_len=sizeof("Message UDP de l'esp32")-1;
	
	size_t bytes_sent=0;
	while(bytes_sent!=tcp_message_len){
		ret=send(client_fd_tcp,tcp_message+bytes_sent,tcp_message_len-bytes_sent,0);
		if(ret<=0){
			puts("L303");
			break;
		}
		bytes_sent+=ret;
	}
	
	bytes_sent=0;
	while(bytes_sent!=udp_message_len){
		ret=send(client_fd_udp,udp_message+bytes_sent,udp_message_len-bytes_sent,0);
		if(ret<=0){
			puts("L313");
			break;
		}
		bytes_sent+=ret;
	}
	*/
	//ret=init_camera();
	
	//ret=sprintf(receive_buff,"init_camera() %i\n",ret);
	//send(client_fd_tcp,receive_buff,ret,0);
	
	/*err=send(client_fd_tcp,"TEXT/BUFFER/TCP",15,0);
	
	if(err<0){
		puts("send() tcp error");
	}
	
	err=send(client_fd_udp,"TEXT/BUFFER/UDP",15,0);
	
	if(err<0){
		puts("send() udp error");
	}
	
	
	err=recv(client_fd_tcp,receive_buff,13,0);
	if(err<0){
		puts("recv() tcp error");
	}
	else{
		puts(receive_buff);
	}
	err=recv(client_fd_udp,receive_buff+64,13,0);
	if(err<0){
		puts("recv() udp error");
	}
	else{
		puts(receive_buff+64);
	}*/
	int val = 1;

	setsockopt(client_fd_tcp, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
	//int *counts=(int*)malloc(1024);
	//if(!counts)	
	//	return;
	size_t packet_size,packet_i=0;
	int r,g,b;
	
	
	ret=init_camera();
				vTaskDelay(pdMS_TO_TICKS(200));
				printf("Entering camera mode B %i\n",ret);
				packet_size=sprintf(receive_buff,"init_camera() %i\n",ret);
				
				
				/*recv(client_fd_tcp, receive_buff+1023, 1, 0);
				if(receive_buff[1023]=='2'){
					puts("HandShake");
				}
				else{
					puts("Nima hendszejka");
				}*/
				//send(client_fd_tcp,"4",1,0);
				printf("client_fd_tcp %i\n",client_fd_tcp);
				packet_i=0;
				while(packet_i!=4){
					ret=send(client_fd_tcp,((char*)&packet_size)+packet_i,4-packet_i,0);
					if(ret<0){
						puts("L385 send() tcp error");
						return;
					}
					packet_i+=ret;
				}	
				puts("Size sent");
				packet_i=0;
				while(packet_i!=packet_size){
					ret=send(client_fd_tcp,receive_buff+packet_i,packet_size-packet_i,0);
					if(ret<0){
						puts("L395 send() tcp error");
						return;
					}
					packet_i+=ret;
				}
				puts("Buff sent");
				
				jpeg_init();
			
				printf("Wolna SRAM: %d bajtow\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

				// Wolna pamięć PSRAM (zewnętrzna - tu masz 8MB w N16R8)
				printf("Wolna PSRAM: %d bajtow\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

				// Największy ciągły blok (ważne, jeśli malloc zwraca NULL mimo wolnego miejsca)
				printf("Największy blok SRAM: %d bajtów\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
				
				pthread_mutex_init(&snapshot_muted, NULL);
			
				/*
				xTaskCreatePinnedToCore(
    (TaskFunction_t)th1_func, // Twoja funkcja (pamiętaj o rzutowaniu)
    "CameraAnalyzer",         // Nazwa dla debuggera
    16384,                    // ROZMIAR STOSU (w bajtach dla S3)
    (void*)client_fd_udp,     // Argument
    5,                        // Priorytet
    NULL,                     // Task handle
    1                         // Rdzeń (Core 1)
		);*/
			esp_task_wdt_delete(NULL);
				puts("th1 created");
				
				
				//th1_command=1;
				
				
				
				
				
				
				while(1){
						//start=clock();
						//if(th1_command==0){
							//puts("th0 attempts to lock mutex");
							//pthread_mutex_lock(&snapshot_muted);
							//puts("th0 locked mutex");
						//}
							
						
						fb = esp_camera_fb_get();
						if(!fb){
							puts("esp_camera_fb_get() failed");
       						send(client_fd_tcp,"\xff\xff\xff\xff",4,0);
							//if(th1_command==0){
								//puts("th0 attempts to unlock mutex");
							//	pthread_mutex_unlock(&snapshot_muted);
								//puts("th0 unlocked mutex");
							//}
							continue;
						}
						//end=clock();
						//printf("snapshot %lli\n",end-start);
						//start=clock();
						//puts("L447");
						ret=recv(client_fd_tcp,receive_buff,1,0);
						if(ret<=0){
							puts("Sync error");
							return;
						}
							//printf("Command : %c\n",*receive_buff);
						switch(*receive_buff){
							case 'P':
								//puts("Pong!");
								break;
							case 'I':
								//puts("0");
								break;
							case 'G':
								//puts("th0 attempts to unlock mutex");
								//if(th1_command==0)
								//	pthread_mutex_unlock(&snapshot_muted);
								//puts("th0 unlocked mutex");
								break;
							case'M':
								//turns on motion detection
								//th1_command=0;
								puts("Motion detection turned on");
								//puts("th0 attempts to unlock mutex");
								//pthread_mutex_unlock(&snapshot_muted);
								//puts("th0 unlocked mutex");
								break;
							case'N':
								//turns off motion detection 
								puts("Motion detection turned off");
								//th1_command=1;
								//analysis_init=0;
								break;
							case'S':
									//esp_camera_fb_return(fb);
									//esp_camera_deinit();
									//fb=NULL;
									//turns off camera feed
									puts("Camera feed turned off");
									//th1_command=1;
									int wait=1;
									while(wait){
										ret=recv(client_fd_tcp,receive_buff,1,0);
										if(ret<=0){
											exit(400);
										}
										switch(*receive_buff){
											case 'G':
												wait=0;
												puts("Camera feed turned on");
												//init_camera();
												//if(th1_command==0)
												//	pthread_mutex_unlock(&snapshot_muted);
												break;
											case'M':
												//turns on motion detection
												puts("Motion detection turned on");
												//th1_command=0;
												break;
											case'N':
												puts("Motion detection turned off");
												//turns off motion detection 
												//th1_command=1;
												//analysis_init=0;
												break;
										}
									}
									//if(th1_command==0)
									//	pthread_mutex_unlock(&snapshot_muted);  //Necessary,but test
							break;
							
							default:
								puts("Unknown command");
							break;
						}
						
						
						
							//end=clock();
						//printf("sync %lli\n",end-start);
    					
							start=clock();

						packet_i=0;
						while(packet_i!=4){
							//puts("L535");
							ret=send(client_fd_tcp,((char*)&fb->len)+packet_i,4-packet_i,0);
							if(ret<=0)
								break;
							packet_i+=ret;
						}
						packet_i=0;
						//memset(counts,0,1024);
						while(packet_i!=fb->len){
							//puts("L545");
							ret=send(client_fd_tcp,fb->buf+packet_i,fb->len-packet_i,0);
							//printf("ret %i\n",ret);
							if(ret<=0)
								break;
							//printf("%u / %u\n",packet_i,fb->len);
							packet_i+=ret;
								//for (size_t i = packet_i-ret; i != packet_i; i++) {
                               //		counts[fb->buf[i]]++;
                            	//}
						}
							//printf("%u / %u\n",fb->len,fb->len);
							//printf("%zu :",fb->len);
							//for (size_t i = 0; i != 256; i++) {
                            ///    	printf(" %zu:%i ",i,counts[i]);
                            //}
                            //puts("");
							
							
							
							end=clock();
							
							
							//printf("sending %lli\n",(fb->len*1000) /(end-start));
							
						//if(th1_command==0){
								//puts("th0 attempts to lock mutex");
						//	pthread_mutex_lock(&snapshot_muted);
								//puts("th0 locked mutex");
						//}
								
						esp_camera_fb_return(fb);
						//if(th1_command==0){
						//	fb=NULL;
								//puts("th0 attempts to unlock mutex");
						//	pthread_mutex_unlock(&snapshot_muted);
								//puts("th0 unlocked mutex");
						//}
								
							//vTaskDelay(pdMS_TO_TICKS(1));
						
						
				}
					
					
				
				
	
	
	
	
	/*
	int tcp_len,packet_length;
	while(1){
		packet_length=recv(client_fd_udp,receive_buff,1024,0);
		printf("packet_length %i\n",packet_length);
		if(packet_length<0)
			puts("UDP recv() error");
			
		if(packet_length==0)
			continue;
		if(packet_length>4){
			if(memcmp(receive_buff,"/tcp/",5)==0){
				//puts("tcp mode");
				err=recv(client_fd_tcp,&tcp_len,4,0);
				if(err<0){
					puts("recv() tcp error");
				}
				err=recv(client_fd_tcp,receive_buff,tcp_len,0);
				if(err<0){
					puts("recv() tcp error");
				}
				else{
					printf("%.*s\n",tcp_len,receive_buff);
				}
			}
			else if((packet_length==8)&&(memcmp(receive_buff,"/camera/",8)==0)){
				
				ret=init_camera();
				printf("Entering camera mode %i\n",ret);
				packet_size=sprintf(receive_buff,"init_camera() %i\n",ret);
				
				recv(client_fd_tcp, receive_buff, 1, 0);
				send(client_fd_tcp,(char*)&packet_size,4,0);
				send(client_fd_tcp,receive_buff,packet_size,0);
				
				while(1){
						fb = esp_camera_fb_get();
   						 if (!fb) {
							puts("esp_camera_fb_get() failed");
       						 send(client_fd_udp,"\xff\xff\xff\xff",4,0);
    					}
    					else{
							//ret=sprintf(receive_buff,"%i",fb->len);
							//printf("sending %i\n",fb->len);
							send(client_fd_udp,&fb->len,4,0);
							//send(client_fd_udp,fb->buf,fb->len,0);
							packet_size=fb->len;
							while(packet_size){
								packet_size=fb->len>1400?1400:fb->len;
								send(client_fd_udp,fb->buf+packet_i,packet_size,0);
								packet_i+=packet_size;
								fb->len-=packet_size;
							}
							//for(size_t i=0;i!=fb->len;i++){
							//	printf("%hhx ",fb->buf[i]);
							//}

							esp_camera_fb_return(fb);
						}						
						vTaskDelay(pdMS_TO_TICKS(10000));
					}
					
					
				
				
			}
			else if((packet_length==8)&&(memcmp(receive_buff,"/camerb/",8)==0)){
				
			}
			else if((memcmp(receive_buff,"/led/",5)==0)){
				if(packet_length==5&&recv(client_fd_udp,receive_buff+5,1019,0)<6)
					puts("led wrong format");
				//puts("/led");
				r=(cast_hex(receive_buff[5])<<4)|cast_hex(receive_buff[6]);
				g=(cast_hex(receive_buff[7])<<4)|cast_hex(receive_buff[8]);
				b=(cast_hex(receive_buff[9])<<4)|cast_hex(receive_buff[10]);
				
				set_led(r,g,b);
			}
			else{
				printf("%.*s\n",packet_length,receive_buff);
			}
		}
		else{
			if(receive_buff[0]=='\x08'){
				break;
			}
			else{
				printf("%.*s\n",packet_length,receive_buff);
			}
		}
	}*/
	
	close(client_fd_tcp);
	close(client_fd_udp);
}

