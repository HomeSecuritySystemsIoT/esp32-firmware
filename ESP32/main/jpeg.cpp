#include"JPEGDEC/JPEGDEC.h"
#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"
#include<new>
JPEGDEC *jpeg=NULL;
uint8_t *grayscale[2];
int gs_select=0,comp_init=0;
#define WIDTH 1280
#define HEIGHT 720
//#include "esp_dsp.h"

extern "C" {
    #include "dsps_sub.h"
    #include "dsps_math.h" 
    #include<stdlib.h>
	#include "esp_camera.h"
	#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include"jpeg.h"
#include "esp_heap_caps.h"
uint32_t calculate_total_diff(uint8_t* buf1, uint8_t* buf2, int len);
}

#define GS_WIDTH WIDTH/8
#define GS_HEIGHT HEIGHT/8


#include<pthread.h>

pthread_mutex_t snapshot_muted;

extern "C" int gray_scale(JPEGDRAW *pdraw){
	uint16_t pixel;
	size_t pos,pos2;
	uint8_t r,g,b;
	for(size_t y=0;y!=pdraw->iHeight;y++){
		for(size_t x=0;x!=pdraw->iWidthUsed;x++){
			pos=y*pdraw->iWidth+x;
		 	pixel = pdraw->pPixels[pos];
		 	//RGB565
			r=(pixel>>11);
			g=(pixel>>5)&0b111111;
			b=pixel&0b11111;
			pixel=(r >> 2) + (g >> 1) + (g >> 3) + (b >> 4);
			
			
			pos2=(y+pdraw->y)*GS_WIDTH+(x+pdraw->x);
			//if(pos2>=GS_WIDTH*GS_HEIGHT){
			//	puts("pos2");
			//	exit(8888);
			//}
			grayscale[gs_select][pos2]=pixel;
		}
		
	}
	return 1;
}


void jpeg_init(){
	void * jpeg_malloc=(JPEGDEC *)heap_caps_malloc(sizeof(JPEGDEC), MALLOC_CAP_SPIRAM);
	if(!jpeg_malloc)
		exit(556);
	jpeg=new (jpeg_malloc)JPEGDEC();
	puts("jpeg allocated!");
}

#include "esp_task_wdt.h"

#include<time.h>

camera_fb_t * fb=NULL;
extern "C" size_t get_diff(camera_fb_t * snap);
int th1_command=2,analysis_init=0;
extern "C" void* th1_func(void* socket){
	uint8_t *buff = (uint8_t *)heap_caps_aligned_alloc(16,GS_WIDTH*GS_HEIGHT*2, MALLOC_CAP_SPIRAM);
	if(!buff)	exit(556);
	puts("TH1 buffers allocated!");
	esp_task_wdt_delete(NULL); // Usuń obecne zadanie z monitoringu Watchdoga
	time_t start,end;
	
	grayscale[0]=buff;
	grayscale[1]=buff+GS_WIDTH*GS_HEIGHT;
	int udp_socket=(int)socket;
	int ret,sent;
	size_t diff;
	UBaseType_t stack_high_water_mark = uxTaskGetStackHighWaterMark(NULL);
	printf("Zapas stosu th1: %u bajtow\n", stack_high_water_mark * 4);
	while(1){
		//puts("TH1 attempts to lock mutex");
		//puts("th1 attempts to lock");
		pthread_mutex_lock(&snapshot_muted);
		
		//puts("TH1 locked mutex");
		if(!fb){		//||th1_command==0 obligatoire
			//puts("(!fb) th1 attempts to unlock");
			pthread_mutex_unlock(&snapshot_muted);
			//puts("(!fb)th1 unlocks");
			vTaskDelay(pdMS_TO_TICKS(20)); // Daj szansę th0!

			continue;
		}
		//puts("TH1 acquired snapshot pointer");
		/*
		switch(th1_command){
			case 1:
				puts("(th1_command==1)th1 attempts to unlock");
				pthread_mutex_unlock(&snapshot_muted);
				puts("(th1_command==1)th1 unlocks");
				return NULL;
			case 2:
				analysis_init=0;
				puts("(th1_command==2)th1 attempts to unlock");
				pthread_mutex_unlock(&snapshot_muted);
				puts("(th1_command==2)th1 unlocks");
				th1_command=1;
				continue;
			break;
		}*/
		if(analysis_init){
			//puts("TH1 attempts get_diff()");
			start=clock();
			diff=get_diff(fb); 
			end=clock();
			//puts("TH1 exits from get_diff()");
			//printf("diff : %zu %lli\n",diff,end-start);
			sent=0;
			while(sent!=4){
				ret=send(udp_socket,&diff,4,0);
				if(ret<=0){
					return NULL;
				}
				sent+=ret;
			}
		}
		else{
			analysis_init=1;
		}
		//puts("th1 attempts to unlock");
		pthread_mutex_unlock(&snapshot_muted);
		//puts("th1 unlocks");
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	
	return NULL;
}








uint32_t calculate_total_diff_unopt(uint8_t* buf1, uint8_t* buf2, int len) {
    uint32_t diff = 0;
    for(int i=0; i!=len; i++) {
        diff += abs(buf1[i] - buf2[i]);
    }
    return diff;
}



extern "C" size_t get_diff(camera_fb_t * snap){
	if(jpeg->openRAM(snap->buf, snap->len, gray_scale)){
		jpeg->setUserPointer(NULL);
    	jpeg->decode(0, 0, JPEG_SCALE_EIGHTH); 
    	jpeg->close();
	}
	if(!comp_init){
		comp_init=1;
		return 0;
	}
	gs_select=!gs_select;
	//printf("Buf1: %p, Buf2: %p, Len: %d\n", grayscale[!gs_select], grayscale[gs_select], GS_WIDTH*GS_HEIGHT);
//calculate_total_diff
	return calculate_total_diff_unopt(
    		(uint8_t*)grayscale[!gs_select], 
   			 (uint8_t*)grayscale[gs_select],GS_WIDTH*GS_HEIGHT);
}