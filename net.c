#include "attack.h"
#include "net.h"
#include <curl/curl.h>

size_t curl_got_highscore_data(void* buffer, size_t size, size_t nmemb, void* userp){
	int i;
	*(((char*)buffer)+nmemb) = '\0';

	printf("parsing retrieved high-score data.. ");
	if ((i=sscanf(buffer,"%d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s %d %s",
	&high_scores[0].points,(char*)&high_scores[0].name,&high_scores[1].points,(char*)&high_scores[1].name,
	&high_scores[2].points,(char*)&high_scores[2].name,&high_scores[3].points,(char*)&high_scores[3].name,
	&high_scores[4].points,(char*)&high_scores[4].name,&high_scores[5].points,(char*)&high_scores[5].name,
	&high_scores[6].points,(char*)&high_scores[6].name,&high_scores[7].points,(char*)&high_scores[7].name,
	&high_scores[8].points,(char*)&high_scores[8].name,&high_scores[9].points,(char*)&high_scores[9].name))!=20){
		printf("error: sscanf got %d\n",i);
	} else {
		high_score_data_is_real = 1;
		printf("success\n");
	}

	for(i=0;i<10;i++){
		printf("score %d: (%d %c%c%c)\n",i,high_scores[i].points,high_scores[i].name[0],high_scores[i].name[1],
		high_scores[i].name[2]);
	}

	return size;
}

//size_t curl_got_updated_binary
