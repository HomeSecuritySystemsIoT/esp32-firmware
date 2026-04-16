
#include <iostream>
#include<stdio.h>
#include<SDL.h>
#include <SDL_image.h>
#include<SDL_ttf.h>
#include<WinSock2.h>
#include<io.h>
struct vstr {
    size_t size;
    char data[];
};
#pragma  warning (disable :4996)
#pragma comment(lib, "ws2_32.lib")

void startTCPServer(SOCKET* listenSock, SOCKET* clientSock,SOCKET udp_socket) {
    *listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (0 > *listenSock) {
        puts("socket() TCP ERROR");
    }
    sockaddr_in serverAddr = { AF_INET, htons(5005), INADDR_ANY };

    int ret =bind(*listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret) {
        printf("bind() error %i\n", ret);
    }

    ret=listen(*listenSock, SOMAXCONN);
    if (ret) {
        printf("listen() error %i\n", ret);
    }
    send(udp_socket, "GO", 2,0);
    puts("Listening");
    *clientSock = accept(*listenSock, NULL, NULL);
    puts("Znaleziono");
}

SOCKET startUDPServer() {
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in serverAddr = { AF_INET, htons(5004), INADDR_ANY };

    int ret = bind(udp_socket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (ret== SOCKET_ERROR) {
        printf("bind() error %i\n", ret);
    }
    sockaddr_in espAddr;
    int espAddrLen = sizeof(espAddr);
    char startBuf[10];

    
    puts("UDP Czekam");
    recvfrom(udp_socket, startBuf, sizeof(startBuf), 0, (sockaddr*)&espAddr, &espAddrLen);
    puts("UDP dostalem");
    connect(udp_socket, (sockaddr*)&espAddr, sizeof(espAddr));
    puts("UDP polaczono");
    /*
    char buf[1024];
    sockaddr_in clientAddr;
    int clientLen = sizeof(clientAddr);
    
    while (true) {
        int bytes = recvfrom(*sock, buf, sizeof(buf), 0, (sockaddr*)&clientAddr, &clientLen);
        if (bytes > 0) {
        }
    }
    */
    return udp_socket;
}
#undef main

void flnm_increment(char* filenm) {
    if (filenm[9] == '9') {
        filenm[9] = '0';
        filenm[8]++;
    }
    else {
        filenm[9]++;
    }
    if (filenm[8] == 0x3a) {
        filenm[8] = '0';
        filenm[7]++;
    }
    if (filenm[7] == 0x3a) {
        filenm[7] = '0';
        filenm[6]++;
    }
    if (filenm[6] == 0x3a) {
        filenm[6] = '0';
    }

}
#include<time.h>


inline void ftostr(int val, SDL_Rect r_draw, SDL_Texture* atlas, SDL_Rect* rects, SDL_Renderer* renderer, int fps) {
    int dec = val % 100, unit = val / 100, itemp = unit;
    int c;
    /*
    while (unit) {
        c = dec % 10;
        r_draw.w = rects[c].w;
        posx += r_draw.w;
        r_draw.h = rects[c].h;
        SDL_RenderCopy(renderer, atlas, rects + c, &r_draw);
        unit /= 10;

    }



    while (dec) {
        c = dec % 10;
        r_draw.w = rects[c].w;
        posx += r_draw.w;
        r_draw.h = rects[c].h;
        SDL_RenderCopy(renderer, atlas, rects+c, &r_draw);
        dec /= 10;
    }
    */

    while (itemp) {
        c = itemp % 10;
        r_draw.w = rects[c].w;
        r_draw.x += r_draw.w;
        itemp /= 10;
    }
    r_draw.w = rects[10].w;
    r_draw.x += r_draw.w;
    if (dec == 0) {
        r_draw.x+= rects[0].w;
    }
    else {
        itemp = dec;
        while (itemp) {
            c = dec % 10;
            r_draw.w = rects[c].w;
            r_draw.x += r_draw.w;
            itemp /= 10;
        }
    }
    


    if (fps) {


        
        r_draw.x += rects[11].w + rects[12].w ;

        //printf("%i %i %i %i -> %i %i %i %i\n", rects[13].x, rects[13].y, rects[13].w, rects[13].h,r_draw.x, r_draw.y, r_draw.w, r_draw.h);

        r_draw.w = rects[13].w;
        r_draw.h = rects[13].h;
        SDL_RenderCopy(renderer, atlas, rects + 13, &r_draw);

        //printf("%i %i %i %i\n", r_draw.x, r_draw.y, r_draw.w, r_draw.h);
        r_draw.x -= rects[12].w;
        r_draw.w = rects[12].w;
        r_draw.h = rects[12].h;
        SDL_RenderCopy(renderer, atlas, rects + 12, &r_draw);

        //printf("%i %i %i %i\n", r_draw.x, r_draw.y, r_draw.w, r_draw.h);
        r_draw.x -= rects[11].w;
        r_draw.w = rects[11].w;
        r_draw.h = rects[11].h;
        SDL_RenderCopy(renderer, atlas, rects + 11, &r_draw);

    }
    else {
        //percent?
    }
   

    if (dec == 0) {
        r_draw.w = rects[0].w;
        r_draw.x -= r_draw.w;
        r_draw.h = rects[0].h;
        SDL_RenderCopy(renderer, atlas, rects, &r_draw);
    }
    else {
        while (dec) {
            c = dec % 10;
            r_draw.w = rects[c].w;
            r_draw.x -= r_draw.w;
            r_draw.h = rects[c].h;
            SDL_RenderCopy(renderer, atlas, rects + c, &r_draw);
            dec /= 10;
        }
    }
    

    r_draw.w = rects[10].w;
    r_draw.x -= r_draw.w;
    r_draw.h = rects[10].h;
    SDL_RenderCopy(renderer, atlas, rects + 10, &r_draw);


    while (unit) {
        c = unit % 10;
        r_draw.w = rects[c].w;
        r_draw.x -= r_draw.w;
        r_draw.h = rects[c].h;
        SDL_RenderCopy(renderer, atlas, rects + c, &r_draw);
        unit /= 10;
    }
    


}




int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_VIDEO);
    int flags = IMG_INIT_JPG | IMG_INIT_PNG;
    if (IMG_Init(flags) != flags) {
        puts("IMG_Init()");
        puts(IMG_GetError());
        exit(-555);
    }
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SDL_Window* wind = SDL_CreateWindow("NXAF", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 850, 0);
    if (!wind)
        return -9;
    //system("pause");
    SDL_Renderer* renderer = SDL_CreateRenderer(wind, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);


    SDL_Surface* temp = IMG_Load("feed_off.png");
    SDL_Texture* tex_feed[4];
    tex_feed[0]= SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    temp = IMG_Load("feed_on.png");
    tex_feed[1] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    if(TTF_Init())
        printf("%s\n", TTF_GetError());

    temp = IMG_Load("feed_off_hoover.png");
    tex_feed[2] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    temp = IMG_Load("feed_on_hoover.png");
    tex_feed[3] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    TTF_Font* font = TTF_OpenFont("ROBOTO.ttf", 40);
    if (!temp) {
        printf("%s\n", TTF_GetError());
    }



    SDL_Rect feed_rect;
    feed_rect.x = 0;
    feed_rect.y = 720;
    feed_rect.w = 240;
    feed_rect.h = 130;


    temp = IMG_Load("md_off.png");
    SDL_Texture* tex_md[4];
    tex_md[0] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    temp = IMG_Load("md_on.png");
    tex_md[1] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);


    temp = IMG_Load("md_off_hoover.png");
    tex_md[2] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    temp = IMG_Load("md_on_hoover.png");
    tex_md[3] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);


    int feed_mode = 1, md_mode = 1;
    SDL_Rect md_rect;
    md_rect.x = 1040;
    md_rect.y = 720;
    md_rect.w = 240;
    md_rect.h = 130;


    SOCKET udp_socket = startUDPServer(),
        tcp_socket_listen,tcp_socket_client;
    
    startTCPServer(&tcp_socket_listen,&tcp_socket_client, udp_socket);
    closesocket(tcp_socket_listen);

    SDL_Texture* tex_bar[4];

    int format;

    temp= IMG_Load("bar00.png");
    tex_bar[0] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    
    SDL_QueryTexture(tex_bar[0], (Uint32*)&format, NULL, NULL,NULL);


    temp = IMG_Load("bar01.png");
    tex_bar[1] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    temp = IMG_Load("bar10.png");
    tex_bar[2] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);


    temp = IMG_Load("bar11.png");
    tex_bar[3] = SDL_CreateTextureFromSurface(renderer, temp);
    SDL_FreeSurface(temp);

    
    SDL_Rect bar_rect;

    bar_rect.x = 240;
    bar_rect.y = 720;
    bar_rect.w = 800;
    bar_rect.h = 130;

    char* fb_buf = (char*)malloc(8000000);
    if (!fb_buf)
        exit(-1);

    char huj[128],input[1025];
    input[1024] = 0;
    /*recv(tcp_socket_client, huj, 128, 0);
    puts(huj);
    recv(udp_socket, huj + 64, 12, 0);
    puts(huj + 64);

    
    send(tcp_socket_client, "zNdN dN NNNN",13,0);
    send(udp_socket, "zNdN dN NNNN",13,0);*/


    SDL_Rect r_draw;

    SDL_Rect copy_rect;
    copy_rect.x = 0;
    copy_rect.y = 0;
    copy_rect.w = 1280;
    copy_rect.h = 720;
    time_t start, end, start2, end2;
    SDL_Event event;
    SDL_Texture* texture = NULL;
    Uint32 ptr_int;
    int ret,input_len;
    int received = 0;
    int remaining = 0;
    char value = 1;
    const float div = 100.0f/(1280.0f * 720.0f);
    setsockopt(tcp_socket_client, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
    int mouse_x, mouse_y;
    char filename[32];
    //memcpy(filename, "output0000.jpg", 15);
    filename[15] = 0;
    int counts[256];
    //int optval = 256 * 1024; // 256 KB
    //setsockopt(tcp_socket_client, SOL_SOCKET, SO_RCVBUF, (char*)&optval, sizeof(optval));
    int last_w, last_h;

    int command = *(int*)"G\0\0\0",last_button=-1;

    int colours[2][3] = {
        0,255,0,
        255,0,0
    };
    SDL_Texture* tex_video_buffer = SDL_CreateTexture(
        renderer,
        format,     // Format koloru (najlepiej pasuje do Blended)
        SDL_TEXTUREACCESS_TARGET,  // Klucz do częstych aktualizacji
        1280,
        720
    ),* numberAtlas = SDL_CreateTexture(
        renderer,
        format,     // Format koloru (najlepiej pasuje do Blended)
        SDL_TEXTUREACCESS_TARGET,  // Klucz do częstych aktualizacji
        512,
        512
    ),*tex_temp;
    SDL_SetTextureBlendMode(numberAtlas, SDL_BLENDMODE_BLEND);

    SDL_Rect fps_rect, motion_rect,fps_copy_rect;

    fps_rect.x = 280;
    fps_rect.y = 740;
    fps_rect.w = 361;
    fps_rect.h = 90;


    motion_rect = fps_rect;
    motion_rect.x = 500;

    SDL_Rect glyphRects[14];
    int posx=0, posy=0,max_height=-1;
    
    SDL_Color fg = { 255,255,255 };
    SDL_SetRenderTarget(renderer, numberAtlas);
    for (int c = '0'; c <= '9'; c++) {
        temp=TTF_RenderGlyph_Blended(font, c, fg);
        if (!temp) {
            printf("%s\n",TTF_GetError());
        }
        tex_temp = SDL_CreateTextureFromSurface(renderer, temp);
        if (posx + temp->w >= 512) { posx = 0; posy += max_height; max_height = -1; }
        max_height = (max_height < temp->h) ? temp->h : max_height;
        int index = c - '0';
        glyphRects[index].x = posx;
        glyphRects[index].y = posy;
        glyphRects[index].w = temp->w;
        glyphRects[index].h = temp->h;
        SDL_RenderCopy(renderer, tex_temp, NULL, glyphRects+ index);
        
        posx += temp->w;
        SDL_FreeSurface(temp);
        SDL_DestroyTexture(tex_temp);
    }
    temp = TTF_RenderGlyph_Blended(font, '.', fg);
    tex_temp = SDL_CreateTextureFromSurface(renderer, temp);
    if (posx + temp->w >= 512) { posx = 0; posy += max_height; max_height = -1; }
    max_height = (max_height < temp->h) ? temp->h : max_height;
    glyphRects[10].x = posx;
    glyphRects[10].y = posy;
    glyphRects[10].w = temp->w;
    glyphRects[10].h = temp->h;
    posx += temp->w;
    SDL_RenderCopy(renderer, tex_temp, NULL, glyphRects + 10);
    SDL_FreeSurface(temp);
    
    temp = TTF_RenderGlyph_Blended(font, 'f', fg);
    tex_temp = SDL_CreateTextureFromSurface(renderer, temp);
    if (posx + temp->w >= 512) { posx = 0; posy += max_height; max_height = -1; }
    max_height = (max_height < temp->h) ? temp->h : max_height;
    glyphRects[11].x = posx;
    glyphRects[11].y = posy;
    glyphRects[11].w = temp->w;
    glyphRects[11].h = temp->h;
    posx += temp->w;
    SDL_RenderCopy(renderer, tex_temp, NULL, glyphRects + 11);
    SDL_FreeSurface(temp);

    temp = TTF_RenderGlyph_Blended(font, 'p', fg);
    tex_temp = SDL_CreateTextureFromSurface(renderer, temp);
    if (posx + temp->w >= 512) { posx = 0; posy += max_height; max_height = -1; }
    max_height = (max_height < temp->h) ? temp->h : max_height;
    glyphRects[12].x = posx;
    glyphRects[12].y = posy;
    glyphRects[12].w = temp->w;
    glyphRects[12].h = temp->h;
    posx += temp->w;
    SDL_RenderCopy(renderer, tex_temp, NULL, glyphRects + 12);
    SDL_FreeSurface(temp);


    temp = TTF_RenderGlyph_Blended(font, 's', fg);
    tex_temp = SDL_CreateTextureFromSurface(renderer, temp);
    if (posx + temp->w >= 512) { posx = 0; posy += max_height; max_height = -1; }
    max_height = (max_height < temp->h) ? temp->h : max_height;
    glyphRects[13].x = posx;
    glyphRects[13].y = posy;
    glyphRects[13].w = temp->w;
    glyphRects[13].h = temp->h;
    SDL_RenderCopy(renderer, tex_temp, NULL, glyphRects + 13);
    SDL_FreeSurface(temp);

    //printf("%i %i %i %i\n", glyphRects[13].x, glyphRects[13].y, glyphRects[13].w, glyphRects[13].h);
    SDL_SetRenderTarget(renderer, NULL);
    //ftostr(1234567, fps_rect, numberAtlas, glyphRects, renderer, 1);
    /*while (1) {
        SDL_PollEvent(&event);
        SDL_RenderCopy(renderer, numberAtlas, NULL, NULL);
        SDL_RenderPresent(renderer);
    }*/
    


    puts("Textures initialised!");
    char float_text[32];
    while (1) {
        //fgets( input,1024,stdin );
        //input_len = strlen(input)-1;
        input_len = _read(0, input, 1024);
        //_write(1, input, input_len);
        //printf("%i\n", input_len);
        if (input_len < 5) {
            //puts("input_len < 5");
            ret = send(udp_socket, input, input_len, 0);
            if (ret < 0)
                printf("UDP_error : %i %i\n", ret, WSAGetLastError());
            continue;
        }
        else {
            if (memcmp(input, "/quit", 5) == 0) {
                ret = send(udp_socket, "\x08", 1, 0);
                if (ret < 0)
                    printf("UDP_error : %i %i\n", ret, WSAGetLastError());
                break;
            }

        }
        
        if (memcmp(input, "/tcp/", 5) == 0) {
            send(udp_socket, "/tcp/", 5, 0);
            input_len -= 5;
            //puts("TCP");
            ret = send(tcp_socket_client,(const char*) &input_len, 4, 0);
            if (ret < 0)
                printf("TCP_error : %i %s\n", ret,WSAGetLastError());


            ret = send(tcp_socket_client, input+5 , input_len, 0);
            if(ret <0)
                printf("TCP_error : %i %s\n", ret, WSAGetLastError());
        }
        else if (memcmp(input, "/camera/", 8) == 0) {
            ret = send(udp_socket, input, 8, 0);
            if (ret < 0)
                printf("UDP_error : %i %s\n", ret, WSAGetLastError());
            ret = recv(udp_socket, input,2, 0);
            ret = recv(tcp_socket_client, input, 1024, 0);
            puts(input);
            while (1) {
                ret=recv(udp_socket, (char*)&ptr_int, 4, 0);
                if (ret < 0) {
                    //puts("recv error");
                    continue;
                }
                else {
                    SDL_PollEvent(&event);
                    printf("%zu\n", ptr_int);
                    if (ptr_int != (Uint32)0xffff) {
                        
                        
                        //recv(udp_socket, fb_buf, ptr_int, 0);
                        /*ptr_int = 0;
                        size_t packet_size, packet_i = 0;
                        while (packet_size= recv(udp_socket, fb_buf + packet_i, 8000000, 0)) {
                            packet_i += packet_size;
                            ptr_int -= packet_size;
                        }*/

                        int remaining = ptr_int;
                       int received = 0;

                        while (remaining > 0) {
                            int r = recv(udp_socket, fb_buf + received, remaining, 0);
                            if (r < 0) {
                                puts("recv() error");
                            }
                            received += r;
                            remaining -= r;
                        }


                        SDL_RWops* rw = SDL_RWFromMem(fb_buf, ptr_int);
                        SDL_Surface* temp = IMG_Load_RW(rw, 1);
                        if (!temp) {
                            continue;
                        }
                        printf("IMG_Load_RW()\n ", temp->format->format);
                        if (texture) {
                            SDL_UpdateTexture(texture, NULL, temp->pixels, temp->pitch);
                        }
                        else {
                            texture = SDL_CreateTextureFromSurface(renderer, temp);
                        }
                        
                        SDL_FreeSurface(temp);
                        SDL_RenderCopy(renderer, texture,NULL,NULL);
                        SDL_RenderPresent(renderer);
                        send(tcp_socket_client, "G", 1, 0);
                    }
                    //printf("%i\n", *ptr_int);
                    
                    
                }
            }
        }
        else if (memcmp(input, "/camerb/", 8) == 0) {
            ret = send(udp_socket, input, 8, 0);
            if (SDL_RenderCopy(renderer, tex_feed[feed_mode], NULL,&feed_rect)) {
                puts("SDL_RenderCopy() error");
            }
            if (SDL_RenderCopy(renderer, tex_md[md_mode], NULL, &md_rect)) {
                puts("SDL_RenderCopy() error");
            }
            SDL_RenderCopy(renderer, tex_bar[3], NULL, &bar_rect);//&



            SDL_RenderPresent(renderer);
            if (ret < 0)
                printf("UDP_error : %i %i\n", ret, WSAGetLastError());
            received = 2;
            received = 0;
            send(tcp_socket_client, "2", 1, 0);
            recv(tcp_socket_client, input, 1, 0);
            if (*input != '4') {
                puts("No Handshake PC part");
            }
            else {
                puts("Handshake PC part");
            }
            while (received !=4) {
                ret = recv(tcp_socket_client, ((char*)&ptr_int)+ received, 4- received, 0);
                if (ret <= 0)
                    continue;
                received += ret;
            }
            received = 0; remaining = ptr_int;
            while (received!=ptr_int) {
                ret = remaining > 1023 ? 1024 : remaining;
                ret = recv(tcp_socket_client, input+ received, ret, 0);
                if (ret <= 0)
                    continue;
                received += ret;
                remaining -= ret;
            }
            input[received] = 0;
            puts(input);




            while (1) {
                
                ret = send(tcp_socket_client,(char*) &command, 1, 0);
                if (ret > 0) {
                    break;
                }
                if (ret < 0)
                    return 1;
            }
            while (1) {
                SDL_PollEvent(&event);
                SDL_GetMouseState(&mouse_x, &mouse_y);
                switch (event.type) {
                case SDL_QUIT:
                    exit(0);
                case SDL_MOUSEBUTTONDOWN:
                    if (mouse_y > 720) {
                        if (mouse_x < 240) {
                            last_button = 1;
                            if (feed_mode) {
                                puts("feed_mode stop");
                                send(tcp_socket_client, "S", 1, 0);
                                SDL_RenderCopy(renderer, tex_feed[0], NULL, &feed_rect);
                            }
                            feed_mode = !feed_mode;
                            if (feed_mode) {
                                puts("feed_mode start");
                                send(tcp_socket_client, "G", 1, 0);
                                SDL_RenderCopy(renderer, tex_feed[1], NULL, &feed_rect);
                            }
                        }
                        if (mouse_x >= 1040) {
                            last_button = 2;
                            md_mode = !md_mode;
                            //command = 'N' - md_mode;
                            if (md_mode) {
                                puts("md_mode on");
                                send(tcp_socket_client, "M", 1, 0);
                                SDL_RenderCopy(renderer, tex_md[1], NULL, &md_rect);
                            }
                            else {
                                puts("md_mode off");
                                send(tcp_socket_client, "N", 1, 0);
                                SDL_RenderCopy(renderer, tex_md[0], NULL, &md_rect);
                            }
                        }
                        SDL_RenderCopy(renderer, tex_bar[md_mode | (feed_mode << 1)], NULL, &bar_rect);
                        last_button = -1;
                    }
                    break;
                default:
                    if (mouse_y > 720) {
                        if (mouse_x < 240) {
                            SDL_RenderCopy(renderer, tex_feed[2+feed_mode], NULL, &feed_rect);
                            
                            if (last_button == 2) {
                                SDL_RenderCopy(renderer, tex_md[md_mode], NULL, &md_rect);
                            }

                            last_button = 1;
                        }
                        else if (mouse_x >= 1040) {
                            SDL_RenderCopy(renderer, tex_md[2+md_mode], NULL, &md_rect);
                            if (last_button == 1) {
                                SDL_RenderCopy(renderer, tex_feed[feed_mode], NULL, &feed_rect);
                            }
                            last_button = 2;
                        }
                        else {
                            switch (last_button) {
                            case 1:
                                SDL_RenderCopy(renderer, tex_feed[feed_mode], NULL, &feed_rect);
                                break;
                            case 2:
                                SDL_RenderCopy(renderer, tex_md[md_mode], NULL, &md_rect);
                                break;
                            }
                            last_button = -1;
                        }
                        
                    }
                    else {
                        switch (last_button) {
                        case 1:
                            SDL_RenderCopy(renderer, tex_feed[feed_mode], NULL, &feed_rect);
                            break;
                        case 2:
                            SDL_RenderCopy(renderer, tex_md[md_mode], NULL, &md_rect);
                            break;
                        }
                        last_button = -1;
                    }
                    SDL_RenderPresent(renderer);
                    break;
                }
                if (!feed_mode) {
                    continue;
                }

                start2 = clock();
                received = 0;
                while (received != 4) {
                    ret = recv(tcp_socket_client, ((char*)&ptr_int) + received, 4- received, 0);
                    if (ret <= 0){
                        puts("recv error");
                    }
                    received += ret;
                }
                    if ((ptr_int != (Uint32)-1)&&(ptr_int<= 8000000)) {
                        received = 0; remaining = ptr_int;
                        memset(counts, 0, sizeof(counts));
                        while (received != ptr_int ) {
                            ret = recv(tcp_socket_client, fb_buf + received, ptr_int- received, 0);
                            if (ret <= 0) {
                                printf("Bląd recv: %d\n", WSAGetLastError());
                                system("pause");
                                return 1;
                            }
                            received += ret;
                        }
                        start = clock();
                        SDL_RWops* rw = SDL_RWFromMem(fb_buf, ptr_int);
                      
                        SDL_Surface* temp = IMG_Load_RW(rw, 1),

                            * temp2 = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888,0);
                        SDL_FreeSurface(temp);
                        temp = temp2;
                        //SDL_RWclose(rw);
                        if (!temp) {
                            printf("Blad IMG: %s\n", IMG_GetError());
                            continue;
                        }
                        while (1) {
                            ret = send(tcp_socket_client, (char*)&command, 1, 0);
                            if (ret > 0) {
                                break;
                            }
                            if (ret <= 0)
                                return 1;
                        }
                        if (texture) {
                            if (last_w == temp->w && last_h == temp->h) {
                                SDL_UpdateTexture(texture, NULL, temp->pixels, temp->pitch);
                            }
                            else {
                                puts("Texture realloc");
                                SDL_DestroyTexture(texture);
                                last_w = temp->w;
                                last_h = temp->h;
                                texture = SDL_CreateTextureFromSurface(renderer, temp);
                            }
                            
                        }
                        else {
                            last_w = temp->w;
                            last_h = temp->h;
                            texture = SDL_CreateTextureFromSurface(renderer, temp);
                        }

                        
                        SDL_FreeSurface(temp);
                        SDL_RenderCopy(renderer, texture, NULL, &copy_rect);
                        
                        
                    }
                    end = clock();
                    received = 0;
                    if(md_mode)
                        ret=recv(udp_socket, (char*)&ptr_int, 4 , 0);
                end2 = clock();
                SDL_RenderCopy(renderer, tex_bar[md_mode | (feed_mode << 1)], NULL, &bar_rect);
                unsigned long long res = (ret == 4) ? ((ptr_int *10000)/1280/720):0;
                ftostr(res, motion_rect, numberAtlas, glyphRects, renderer, 0);
                printf("%llu %f\n", res, float(ptr_int)* div);
                res =100000/ (end2 - start2) ;
                ftostr(res, fps_rect, numberAtlas, glyphRects, renderer, 1);
                
                SDL_RenderPresent(renderer);
            }

        }
        else {
            ret = send(udp_socket, input, input_len, 0);
            if (ret < 0)
                printf("UDP_error : %i %s\n", ret, WSAGetLastError());
        }
       
    }




    closesocket(tcp_socket_client);
    closesocket(udp_socket);
    system("pause");
}

