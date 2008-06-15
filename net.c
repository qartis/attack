#include "attack.h"
#include "net.h"
#include <curl/curl.h>

double ver = 
#include ".ver"
;

size_t curl_got_highscore_data(void* buffer, size_t size, size_t nmemb, void* userp){
        char newurl[128];
	int i;
        double cur_ver;
	*(((char*)buffer)+nmemb) = '\0';

//	printf("parsing retrieved high-score data.. '%s'\n",(char*)buffer);
        if (0==strncmp(buffer,"err",3)){
            printf("protocol error\n");
            exit(0);
        }
	if ((i=sscanf(buffer,"%d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s %lf %s",
	&high_scores[0].points,(char*)&high_scores[0].name,&high_scores[1].points,(char*)&high_scores[1].name,
	&high_scores[2].points,(char*)&high_scores[2].name,&high_scores[3].points,(char*)&high_scores[3].name,
	&high_scores[4].points,(char*)&high_scores[4].name,&high_scores[5].points,(char*)&high_scores[5].name,
	&high_scores[6].points,(char*)&high_scores[6].name,&high_scores[7].points,(char*)&high_scores[7].name,
	&high_scores[8].points,(char*)&high_scores[8].name,&high_scores[9].points,(char*)&high_scores[9].name,&cur_ver,newurl))!=22){
		printf("error: sscanf got %d\n",i);
	} else {
		high_score_data_is_real = 1;
		printf("success\n");
                if (cur_ver != ver){
                    printf("version mismatch\n");
                    if (ver < cur_ver){
                        printf("new version: %s\n",newurl);
                        strncpy(url,newurl,strlen(newurl));
                    } else {
                        printf("newer version than current? you %lf cur %lf\n",ver,cur_ver);
                    }
                }
	}

	for(i=0;i<10;i++){
		printf("score %d: (%6d %c%c%c)\n",i+1,high_scores[i].points,high_scores[i].name[0],high_scores[i].name[1],
		high_scores[i].name[2]);
	}

	return size;
}

void highscores(){
//    CURLM *multi;
    int i;
    int m_was_pressed = 0;
    CURL *easy;
    Uint8 *keys;
    SDL_Event event;
    char *hs = "UPDATING SCORES";
    SDL_FillRect(screen,NULL,colours[0]);
    SFont_Write(screen,font[1],(SCREEN_WIDTH-SFont_TextWidth(font[0],hs))/2,(SCREEN_HEIGHT-SFont_TextHeight(font[0]))/2,hs);
    SDL_UpdateRect(screen,0,0,0,0);
//    multi = curl_multi_init();
    easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_URL, "http://h.qartis.com/attack/scores.php");
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curl_got_highscore_data);
    char *data = malloc(128);
    sprintf(data,"n=%d&n1=%c%c%c&s1=%d&n2=%c%c%c&s2=%d&v=%lf",num_players,
            player_scores[0].name[0],player_scores[0].name[1],player_scores[0].name[2],player_scores[0].points,
            player_scores[1].name[0],player_scores[1].name[1],player_scores[1].name[2],player_scores[1].points,ver);
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 10);
//    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 0);
//    curl_multi_add_handle(multi,easy);
    curl_easy_perform(easy);
refresh:
    SDL_FillRect(screen,NULL,colours[0]);
    if (high_score_data_is_real){
        SFont_Write(screen,font[4],(SCREEN_WIDTH-SFont_TextWidth(font[0],"HIGH SCORES"))/2,135,"HIGH SCORES");
        for(i=0;i<10;i++){
            char buf[2][32];
            sprintf(buf[0],"%6d",high_scores[i].points);
            sprintf(buf[1],"%c%c%c",high_scores[i].name[0],high_scores[i].name[1],high_scores[i].name[2]);
            SFont_Write(screen,font[5],(SCREEN_WIDTH)/2-SFont_TextWidth(font[0],buf[0]),
                        HIGHSCORES_STARTHEIGHT+i*(SFont_TextHeight(font[0])+5),buf[0]);
            SFont_Write(screen,font[5],(SCREEN_WIDTH)/2+10,
                        HIGHSCORES_STARTHEIGHT+i*(SFont_TextHeight(font[0])+5),buf[1]);
        }
        if (url[0] != '\0'){
            SFont_Write(screen,font[2],(SCREEN_WIDTH-SFont_TextWidth(font[0],"NEW VERSION"))/2,450,"NEW VERSION");
            SFont_Write(screen,font[2],(SCREEN_WIDTH-SFont_TextWidth(font[0],url))/2,460,url);
        }
    } else {
        char *message = "CONNECTION FAILED";
        SFont_Write(screen,font[3],(SCREEN_WIDTH-SFont_TextWidth(font[0],message))/2,(SCREEN_HEIGHT-SFont_TextHeight(font[0]))/2,message);
    }
    SDL_UpdateRect(screen,0,0,0,0);
    for(;;){
        if (!SDL_WaitEvent(&event)) return;
            if (event.type == SDL_QUIT){
                requests_quit = 1;
                return;
            }
        keys = SDL_GetKeyState(NULL);
        if (keys[SDLK_q] == SDL_PRESSED){
            requests_quit = 1;
            return;
        }
        if (keys[SDLK_ESCAPE] == SDL_PRESSED ||
            keys[SDLK_RETURN] == SDL_PRESSED ||
            keys[SDLK_SPACE] == SDL_PRESSED) return;
        if (keys[SDLK_f] == SDL_PRESSED){
            vidmode();
            goto refresh;
        }
        if (keys[SDLK_m] == SDL_PRESSED){
            if (!m_was_pressed){
                toggle_mute();
                m_was_pressed = 1;
            }
        } else {
            m_was_pressed = 0;
        }
//        SDL_Delay(100);
    }
}


//size_t curl_got_updated_binary
