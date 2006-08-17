#include "attack.h"

void setpixel(SDL_Surface *surface, int x, int y, Uint32 pixel){
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
    *(Uint32 *)p = pixel;
}

Uint32 getpixel(SDL_Surface *surface, int x, int y){
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	return *(Uint32 *)p;
}

SDL_Surface* tint(SDL_Surface *sprite, Uint32 to){
	int i,j;
	SDL_Surface *new;
	new = SDL_ConvertSurface(sprite, sprite->format, sprite->flags);

	Uint32 white = SDL_MapRGB(new->format,0xFF,0xFF,0xFF);

	SDL_LockSurface(new);
	for(i=0;i<new->h;i++){
		for(j=0;j<new->w;j++){
			if (getpixel(new,j,i) != white) setpixel(new,j,i,to);
		}
	}
	SDL_UnlockSurface(new);
	return new;
}

size_t curl_write_data(void* buffer, size_t size, size_t nmemb, void* userp){
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


SDL_Surface *load_image(char *datafile, int transparent){
	SDL_Surface *image, *surface;

	image = IMG_Load(datafile);
	if ( image == NULL ) {
		fprintf(stderr, "Couldn't load image %s: %s\n",
					datafile, IMG_GetError());
		return(NULL);
	}
	if ( transparent ) {
		SDL_SetColorKey(image, SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(image->format,0xFF,0xFF,0xFF));
	}
	surface = SDL_DisplayFormat(image);
	SDL_FreeSurface(image);
	return(surface);
}

int load_data(void){
	int i;

	/* Load sounds */
	sounds[MUSIC_WAV] = Mix_LoadWAV(DATAFILE("music.wav"));
	sounds[SHOT_WAV] = Mix_LoadWAV(DATAFILE("shot.wav"));
	sounds[EXPLODE_WAV] = Mix_LoadWAV(DATAFILE("kaboom.wav"));

	/* Load graphics */
	player.image = load_image(DATAFILE("player.gif"), 1);
	if ( player.image == NULL ){
		printf("player.image == NULL\n");
		return(0);
	}

	colours[0] = SDL_MapRGB(screen->format,0x00,0x00,0x00); //black
	colours[1] = SDL_MapRGB(screen->format,0xA7,0x40,0x40); //brown
	colours[2] = SDL_MapRGB(screen->format,0xFF,0x00,0x00); //red
	colours[3] = SDL_MapRGB(screen->format,0xFF,0x99,0x00); //orange
	colours[4] = SDL_MapRGB(screen->format,0xFF,0xFF,0x00); //yellow
	colours[5] = SDL_MapRGB(screen->format,0x00,0xFF,0x00); //green
	colours[6] = SDL_MapRGB(screen->format,0x00,0x00,0xFF); //blue
	colours[7] = SDL_MapRGB(screen->format,0xD0,0x00,0xD0); //violet
	colours[8] = SDL_MapRGB(screen->format,0xB0,0xB0,0xB0); //grey
	colours[9] = SDL_MapRGB(screen->format,0xFE,0xFF,0xFF); //white

	shot_images[0]     = load_image(DATAFILE("shot0.gif"), 0);
	enemy_images[0][0] = load_image(DATAFILE("enemy_0.png"), 1);
	enemy_images[0][1] = load_image(DATAFILE("enemy_1.png"), 1);

	for(i=1;i<10;i++){
		shot_images[i] = tint(shot_images[0],colours[i]);
	}

	//black doesn't need tint
	for(i=1;i<10;i++){
		enemy_images[i][0] = tint(enemy_images[0][0],colours[i]);
		enemy_images[i][1] = tint(enemy_images[0][1],colours[i]);
	}

	for(i=0;i<10;i++){
		if (shot_images[i] == NULL){
			printf("failed to load shot_images[%d]\n",i);
			return(0);
		}
	}

	enemy_shot_images[0][0]=load_image(DATAFILE("enemy_shot_0_0.png"),1);
	enemy_shot_images[0][1]=load_image(DATAFILE("enemy_shot_0_1.png"),1);
	enemy_shot_images[1][0]=load_image(DATAFILE("enemy_shot_1_0.png"),1);
	enemy_shot_images[1][1]=load_image(DATAFILE("enemy_shot_1_1.png"),1);

	for(i=0;i<MAX_ENEMY_SHOTS;i++) enemy_shots[i].image = enemy_shot_images[0][0];

	for (i=1;i<MAX_SHOTS;i++) shots[i].image = shot_images[shots[i].colour];

	enemies[0].image = enemy_images[0][0];
	if (enemies[0].image==NULL) return(0);
	for (i=1;i<MAX_ENEMIES;i++) enemies[i].image = enemies[0].image;

	explosions[0].image = load_image(DATAFILE("blam.gif"), 1);

	for (i=1;i<MAX_ENEMIES+1;i++) explosions[i].image = explosions[0].image;

	title_screen = load_image(DATAFILE("title_screen.png"), 0);

	for(i=0;i<3;i++) castle[i] = load_image(DATAFILE("castle.png"),1);

	castles[0].image = castle[0];
	castles[1].image = castle[1];
	castles[2].image = castle[2];

	ic_image = load_image(DATAFILE("ic.png"),1);

	font[0] = SFont_InitFont(IMG_Load(DATAFILE("font-bluish.png")));
	font[1] = SFont_InitFont(IMG_Load(DATAFILE("font-blue.png")));
	font[2] = SFont_InitFont(IMG_Load(DATAFILE("font-yellow.png")));
	font[3] = SFont_InitFont(IMG_Load(DATAFILE("font-red.png")));
	font[4] = SFont_InitFont(IMG_Load(DATAFILE("font-white.png")));
	font[5] = SFont_InitFont(IMG_Load(DATAFILE("font-green.png")));
	font[6] = SFont_InitFont(IMG_Load(DATAFILE("font-grey.png")));

	for(i=0;i<6;i++) if (!font[i]) return 0;

	points_lookup[0] = 1000; //black
	points_lookup[1] = 750; //brown
	points_lookup[2] = 500; //red
	points_lookup[3] = 400; //orange
	points_lookup[4] = 350; //yellow
	points_lookup[5] = 300; //green
	points_lookup[6] = 250; //blue
	points_lookup[7] = 200; //violet
	points_lookup[8] = 150;  //grey
	points_lookup[9] = 50;  //white

	return(1);
}

int rand_between(int low, int high){
	return rand() % (high - low + 1) + low;
}

int can_shoot(){
	int i;
	for (i=0; i<MAX_SHOTS; i++) {
		if (!shots[i].alive) return i;
	}
	return -1;
}

void setfourpixels(object *castle, int x, int y){
	Uint32 white = SDL_MapRGB(castle->image->format,0xFF,0xFF,0xFF);

	y-=y%2;
	x-=x%2;

	if (y < 0) y = -y;

	if (x < 0 || x >= castle->image->w-1 || y < 0 || y >= castle->image->h-1) return;

	setpixel(castle->image,x,y,white);
	setpixel(castle->image,x+1,y,white);
	setpixel(castle->image,x,y+1,white);
	setpixel(castle->image,x+1,y+1,white);
}

void castle_hit(object *castle, int x, int y, int down){
	int i;
	
	x-=castle->x;

	if (down) y-=castle->y-12;
	else      y-=castle->y;

	SDL_LockSurface(castle->image);

	for(i=0;i<60;i++){
		setfourpixels(castle,x+rand_between(1,10)-5,y+rand_between(1,10)-5);
	}

	for(i=0;i<40;i++){
		setfourpixels(castle,x+rand_between(1,16)-8,y+rand_between(1,16)-8);
	}

	SDL_UnlockSurface(castle->image);
}


void FreeData(void){
	int i;

	for ( i=0; i<NUM_WAVES; ++i ) Mix_FreeChunk(sounds[i]);

	SDL_FreeSurface(player.image);
	for(i=0;i<10;i++){
		SDL_FreeSurface(shot_images[i]);
		SDL_FreeSurface(enemy_images[i][0]);
		SDL_FreeSurface(enemy_images[i][1]);
	}
	SDL_FreeSurface(explosions[0].image);
	SDL_FreeSurface(title_screen);
	for(i=0;i<6;i++) SFont_FreeFont(font[i]);
}

void DrawObject(object *sprite){
	SDL_Rect a;
	a.x=sprite->x;
	a.y=sprite->y;
	SDL_BlitSurface(sprite->image,NULL,screen,&a);
}

int pixel_collision(object *sprite1, object *sprite2, int bottom){
//	checks if $bottom edge of sprite1 hits pixels of sprite2

	Uint32 pix;
	int x,y,i;
	Uint8 r,g,b;

	if (bottom){
		y = sprite1->y + sprite1->image->h;
		if (y-sprite2->y < 0 || y-sprite2->y >= sprite2->image->h) return 0;
	} else {
		y = sprite1->y;
		if (y-sprite2->y < 0 || y-sprite2->y >= sprite2->image->h) return 0;
	}

	for(i=0;i<sprite1->image->w;i++){
		x = sprite1->x+i;
		if (x-sprite2->x > 0 && x-sprite2->x < sprite2->image->w){
			SDL_LockSurface(sprite2->image);
			pix = getpixel(sprite2->image,x-sprite2->x,y-sprite2->y);
			SDL_UnlockSurface(sprite2->image);
			SDL_GetRGB(pix,sprite2->image->format,&r,&g,&b);
			if (r+g+b != 3*255) return 1;
		}
	}
	return 0;
}

int box_collision(object *sprite1, object *sprite2){
	if ( (sprite1->y >= (sprite2->y+sprite2->image->h)) ||
	     (sprite1->x >= (sprite2->x+sprite2->image->w)) ||
	     (sprite2->y >= (sprite1->y+sprite1->image->h)) ||
	     (sprite2->x >= (sprite1->x+sprite1->image->w)) ) {
		return(0);
	}
	return(1);
}

int dead_shot(){
	int i;
	for(i=0;i<MAX_ENEMY_SHOTS;i++){
		if (!enemy_shots[i].alive) return i;
	}
	return -1;
}

int find_enemy_who_can_shoot(){
	int i,j;
	int available_enemies[ENEMY_COLUMNS];
	int num_available=0;

	for(i=0;i<ENEMY_COLUMNS;i++){
		for(j=ENEMY_ROWS-1;j>-1;j--){
			if (enemies[j*ENEMY_COLUMNS+i].alive){
				available_enemies[num_available++] = j*ENEMY_COLUMNS+i;
				break;
			}
		}
	}

	if (num_available < 1) return -1;
	return available_enemies[rand_between(0,num_available-1)];
}

int need_reverse_enemies(){
	printf("checking need to reverse\n");
	int i;
	for(i=0;i<MAX_ENEMIES;i++){
		if (enemies[i].x+(enemy_direction*ENEMY_SPEED)   < 0 ||
		   (enemies[i].x+(enemy_direction*ENEMY_SPEED)) >= screen->w-enemies[i].image->w){
			printf("yes, reversing\n");
			return 1;
		}
	}
	printf("determined that I don't need to reverse\n");
	return 0;
}

void player_hit(){
	lives--;
	player.alive = PLAYER_DEATH_DELAY;
/*	if (lives<1){
		printf("dying\n");
	} else {
		printf("hit %d remain\n",lives);
	} */
}

void wait(void){
	static Uint32 next_tick = 0;
	Uint32 this_tick;
	/* Wait for the next frame */
	this_tick = SDL_GetTicks(); 
	if (this_tick<next_tick) SDL_Delay(next_tick-this_tick);
	next_tick = this_tick + (1000/FRAMES_PER_SEC);
}

void draw_points(int disabled){
	SFont_Font* player1_name;
	SFont_Font* player1_points;
	SFont_Font* player2_name;
	SFont_Font* player2_points;

	if (disabled == 1){
		player1_name = font[6];
		player1_points = font[6];
		player2_name = font[6];
		player2_points = font[6];
	} else {
		if (current_player == 1){
			player1_name = font[1];
			player1_points = font[1];
			player2_name = font[0];
			player2_points = font[4];
		} else {
			player1_name = font[0];
			player1_points = font[4];
			player2_name = font[1];
			player2_points = font[1];
		}
	}

	SFont_Write(screen,player1_points,6,5,"SCORE-1");

	char buf[7];
	sprintf(buf,"%.6d",player_scores[0].points);
	SFont_Write(screen,player1_points,9,17,buf);

	if (num_players == 2){
		SFont_Write(screen,player2_points,SCREEN_WIDTH-6-SFont_TextWidth(font[2],"SCORE-2"),5,"SCORE-2");
		sprintf(buf,"%.6d",player_scores[1].points);
		SFont_Write(screen,player2_points,SCREEN_WIDTH-9-SFont_TextWidth(font[2],buf),17,buf);
	}

	SFont_Write(screen,font[3],(SCREEN_WIDTH-SFont_TextWidth(font[3],"TAITO"))/2,6,"TAITO");


	SFont_Write(screen,font[4],(SCREEN_WIDTH-SFont_TextWidth(font[4],"CREDIT 00")) - 40,SCREEN_HEIGHT-20,"CREDIT 00");
}

void show_title_screen(void){
	SDL_Event event;
	Uint8 *keys;

	current_player = (num_players==2)?1:0;

	for(;;){
                while (SDL_PollEvent(&event)){
                        if (event.type == SDL_QUIT){
				requests_quit = 1;
				return;
			}
                }

		SDL_Rect dst;
		dst.x = 0;
		dst.y = 0;
		dst.w = SCREEN_WIDTH;
		dst.h = SCREEN_HEIGHT;
		SDL_BlitSurface(title_screen,NULL,screen,&dst);

		draw_points(1);

		if (current_player == 0){
			SFont_Write(screen,font[0],(screen->w - SFont_TextWidth(font[0],"1 PLAYER"))/2,340,"1 PLAYER");
			SFont_Write(screen,font[1],(screen->w - SFont_TextWidth(font[1],"2 PLAYER"))/2,360,"2 PLAYERS");
		} else {
			SFont_Write(screen,font[1],(screen->w - SFont_TextWidth(font[0],"1 PLAYER"))/2,340,"1 PLAYER");
			SFont_Write(screen,font[0],(screen->w - SFont_TextWidth(font[1],"2 PLAYER"))/2,360,"2 PLAYERS");
		}

		keys = SDL_GetKeyState(NULL);
		if (keys[SDLK_q] == SDL_PRESSED){
			requests_quit = 1;
			return;
		} else if (keys[SDLK_DOWN] == SDL_PRESSED){
			current_player = 1;
			num_players = 2;
		} else if (keys[SDLK_UP] == SDL_PRESSED){
			current_player = 0;
			num_players = 1;
		} else if (keys[SDLK_RETURN] == SDL_PRESSED) return;
		else if (keys[SDLK_1] == SDL_PRESSED){
			current_player = 0;
			return;
		} else if (keys[SDLK_2] == SDL_PRESSED){
			current_player = 1;
			return;
		}
		SDL_UpdateRect(screen, 0, 0, 0, 0);
		SDL_Delay(20);
	}
}

void end_screen(void){
	SDL_Event event;
	Uint8 *keys;

	for(;;){
                while (SDL_PollEvent(&event)){
                        if (event.type == SDL_QUIT){
				requests_quit = 1;
				printf("quitting\n");
				return;
			}
                }

		SDL_Rect dst;
		dst.x = 0;
		dst.y = 0;
		dst.w = SCREEN_WIDTH;
		dst.h = SCREEN_HEIGHT;
		SDL_BlitSurface(title_screen,NULL,screen,&dst);

		if (current_player == 0){
			SFont_Write(screen,font[0],(screen->w - SFont_TextWidth(font[0],"1 PLAYER"))/2,340,"1 PLAYER");
			SFont_Write(screen,font[1],(screen->w - SFont_TextWidth(font[1],"2 PLAYER"))/2,360,"2 PLAYERS");
		} else {
			SFont_Write(screen,font[1],(screen->w - SFont_TextWidth(font[0],"1 PLAYER"))/2,340,"1 PLAYER");
			SFont_Write(screen,font[0],(screen->w - SFont_TextWidth(font[1],"2 PLAYER"))/2,360,"2 PLAYERS");
		}

		keys = SDL_GetKeyState(NULL);
		if (keys[SDLK_q] == SDL_PRESSED){
			return;
		} else if (keys[SDLK_DOWN] == SDL_PRESSED){
			current_player = 1;
			num_players = 2;
		} else if (keys[SDLK_UP] == SDL_PRESSED){
			current_player = 0;
			num_players = 1;
		} else if (keys[SDLK_RETURN] == SDL_PRESSED) return;
		else if (keys[SDLK_1] == SDL_PRESSED){
			current_player = 0;
			return;
		} else if (keys[SDLK_2] == SDL_PRESSED){
			current_player = 1;
			return;
		}
		SDL_UpdateRect(screen, 0, 0, 0, 0);
		SDL_Delay(20);
	}
}

void game(void){
	int i,j;
	Uint8 *keys;
	SDL_Event event;

	player.alive = 1;
	player.x = (screen->w - player.image->w)/2;
	player.y = (screen->h - player.image->h) - 1 - PLAYER_HEIGHT;
	player.facing = 0;

	for (i=0;i<MAX_SHOTS;i++){
		shots[i].alive = 0;
	}

	for(i=0;i<ENEMY_COLUMNS;i++){
		enemies[0*ENEMY_COLUMNS+i].colour = rand_between(0,1);
		enemies[1*ENEMY_COLUMNS+i].colour = rand_between(2,3);
		enemies[2*ENEMY_COLUMNS+i].colour = rand_between(4,5);
		enemies[3*ENEMY_COLUMNS+i].colour = rand_between(6,7);
		enemies[4*ENEMY_COLUMNS+i].colour = rand_between(8,9);
	}

	for(i=0;i<ENEMY_ROWS;i++){
		for(j=0;j<ENEMY_COLUMNS;j++){
			int n = i*ENEMY_COLUMNS + j;
			enemies[n].alive = 1;
			enemies[n].y = enemies[n].image->h*i + ENEMY_OFFSET;
			enemies[n].x = enemies[n].image->w*j;
			enemies[n].image = enemy_images[enemies[n].colour][0];
		}
	}

	for(i=0;i<MAX_ENEMY_SHOTS;i++) enemy_shots[i].alive = 0;

	enemy_direction = 1;
	chomp_counter = 0;
	chomp_frame = 0;
	enemy_shot_counter = 0;
	enemy_shot_frame = 0;
	music_counter = 0;
	music_speed = 60;
	enemy_movement_counter = 0;
	current_player = 0;
	lives = LIVES;

	i=castles[0].image->w/2;

	castles[0].x = 1*(SCREEN_WIDTH/4) - i;
	castles[1].x = 2*(SCREEN_WIDTH/4) - i;
	castles[2].x = 3*(SCREEN_WIDTH/4) - i;
	castles[0].y = SCREEN_HEIGHT - 2*castles[0].image->h - CASTLE_HEIGHT;
	castles[1].y = castles[0].y;
	castles[2].y = castles[0].y;


	while (lives>0) {
		wait();
		while (SDL_PollEvent(&event)){
			if (event.type==SDL_QUIT) return;
		}
		keys = SDL_GetKeyState(NULL);

		if (keys[SDLK_q] == SDL_PRESSED){ requests_quit = 1; return; }

		/* Paint the background */
//		SDL_Rect dst;
//		dst.x = 0;
//		dst.y = 0;
//		SDL_BlitSurface(background, NULL, screen, &dst);
		SDL_FillRect(screen,NULL,colours[0]);

		/* Decrement the lifetime of the explosions */
		for (i=0;i<MAX_ENEMIES+1;i++){
			if (explosions[i].alive>0) explosions[i].alive--;
		}

		/* Create new shots */
		if ((i=can_shoot())!=-1){
			if ( keys[SDLK_0] == SDL_PRESSED ) {
				shots[i].alive = 1;
				shots[i].colour = 0;
				shots[i].image = shot_images[0];
			} else if (keys[SDLK_1] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 1;
				shots[i].image = shot_images[1];
			} else if (keys[SDLK_2] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 2;
				shots[i].image = shot_images[2];
			} else if (keys[SDLK_3] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 3;
				shots[i].image = shot_images[3];
			} else if (keys[SDLK_4] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 4;
				shots[i].image = shot_images[4];
			} else if (keys[SDLK_5] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 5;
				shots[i].image = shot_images[5];
			} else if (keys[SDLK_6] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 6;
				shots[i].image = shot_images[6];
			} else if (keys[SDLK_7] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 7;
				shots[i].image = shot_images[7];
			} else if (keys[SDLK_8] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 8;
				shots[i].image = shot_images[8];
			} else if (keys[SDLK_9] == SDL_PRESSED){
				shots[i].alive = 1;
				shots[i].colour = 9;
				shots[i].image = shot_images[9];
			}

			if (shots[i].alive==1){
				printf("made a shot\n");
				shots[i].x = player.x+(player.image->w-shots[i].image->w)/2;
				shots[i].y = player.y-shots[i].image->h;
				Mix_PlayChannel(SHOT_WAV,sounds[SHOT_WAV], 0);
			}
		}

		/* Move the player */
		player.facing = 0;
		if (keys[SDLK_RIGHT]) player.facing++;
		if (keys[SDLK_LEFT])  player.facing--;

		player.x += player.facing*PLAYER_SPEED;

		if ( player.x < 0 ) player.x = 0;
		else if (player.x >= (screen->w-player.image->w)){
			 player.x  = (screen->w-player.image->w)-1;
		}

		if (chomp_counter > CHOMP_SPEED){
			chomp_frame = (chomp_frame+1)%2;
			for(i=0;i<MAX_ENEMIES;i++){
				enemies[i].image = enemy_images[enemies[i].colour][chomp_frame];
			}
			chomp_counter = 0;
		} else chomp_counter++;

		if (music_counter > music_speed){
			music_counter = 0;
			Mix_PlayChannel(SHOT_WAV,sounds[SHOT_WAV], 0);
		} else music_counter++;

		/* Detect new enemy shots */
		if (rand_between(1,ENEMY_SHOT_CHANCE) == ENEMY_SHOT_CHANCE){
			if ((i=dead_shot())!=-1){
				if ((j=find_enemy_who_can_shoot())!=-1){
					enemy_shots[i].colour = rand_between(0,1);
					enemy_shots[i].image = enemy_shot_images[enemy_shots[i].colour][enemy_shot_frame];
					enemy_shots[i].alive = 1;
					enemy_shots[i].x = enemies[j].x+(enemies[j].image->w/2);
					enemy_shots[i].y = enemies[j].y+enemies[j].image->h;
				}
			}
		}

		/* Move the enemies' shots */
		if (enemy_shot_counter > ENEMY_SHOT_FLICKER){
			enemy_shot_frame = (enemy_shot_frame+1)%2;
			enemy_shot_counter = 0;
		} else {
			enemy_shot_counter++;
		}

		for(i=0;i<MAX_ENEMY_SHOTS;i++){
			if (enemy_shots[i].alive){
				enemy_shots[i].y+=ENEMY_SHOT_SPEED;
				enemy_shots[i].image = enemy_shot_images[enemy_shots[i].colour][enemy_shot_frame];
			}
		}


		/* Move the enemies */
		if (enemy_movement_counter > ENEMY_MOVE_RATE){
			enemy_movement_counter=0;

			if (need_reverse_enemies()){
				enemy_direction = -enemy_direction;
				printf("enemy direction is now %d\n",enemy_direction);
				for (i=0;i<MAX_ENEMIES;i++){
					if (enemies[i].alive) enemies[i].y+=enemies[i].image->h;
				}
				printf("moved all the enemies down\n");
			} else {
				for(i=0;i<MAX_ENEMIES;i++){
					if (enemies[i].alive) enemies[i].x+=enemy_direction*ENEMY_SPEED;
				}
				printf("didn't need to reverse, so moved them all forward\n");
			}
		} else {
			enemy_movement_counter++;
		}

		/* Move the shots */
		for ( i=0; i<MAX_SHOTS; ++i ) {
			if ( shots[i].alive ) {
				shots[i].y -= SHOT_SPEED;
				if ( shots[i].y < SHOT_MAXHEIGHT ) {
					shots[i].alive = 0;
				}
			}
		}

		/* SHOTS AND CASTLES */
		for(i=0;i<MAX_SHOTS;i++){
			if (!shots[i].alive) continue;
			if (pixel_collision(&shots[i],&castles[0],0)){
				castle_hit(&castles[0],shots[i].x,shots[i].y,0);
				shots[i].alive=0;
				continue;
			} else if (pixel_collision(&shots[i],&castles[1],0)){
				castle_hit(&castles[1],shots[i].x,shots[i].y,0);
				shots[i].alive=0;
				continue;
			} else if (pixel_collision(&shots[i],&castles[2],0)){
				castle_hit(&castles[2],shots[i].x,shots[i].y,0);
				shots[i].alive=0;
				continue;
			}
		}

		/* SHOTS AND ALIENS */
		for ( j=0; j<MAX_SHOTS; ++j ) {
			for ( i=0; i<MAX_ENEMIES; ++i ) {
				if ( shots[j].alive && enemies[i].alive && shots[j].colour == enemies[i].colour && box_collision(&shots[j],&enemies[i])) {
					enemies[i].alive = 0;
					explosions[i].x = enemies[i].x;
					explosions[i].y = enemies[i].y;
					explosions[i].alive = EXPLODE_TIME;
//					explosions[i].image = tint(explosions[i].image,colours[enemies[i].colour]);
					Mix_PlayChannel(EXPLODE_WAV,sounds[EXPLODE_WAV], 0);
					shots[j].alive = 0;
					player_scores[current_player].points += points_lookup[enemies[i].colour];
					break;
				}
			}
		}

		/* ENEMIES AND PLAYER */
		for ( i=0; i<MAX_ENEMIES; ++i ) {
			if ( enemies[i].alive && box_collision(&player, &enemies[i]) ) {
				enemies[i].alive = 0;
				explosions[i].x = enemies[i].x;
				explosions[i].y = enemies[i].y;
				explosions[i].alive = EXPLODE_TIME;
				lives = 0;
				explosions[MAX_ENEMIES].x = player.x;
				explosions[MAX_ENEMIES].y = player.y;
				explosions[MAX_ENEMIES].alive = EXPLODE_TIME;
				Mix_PlayChannel(EXPLODE_WAV,sounds[EXPLODE_WAV], 0);
			}
		}

		/* ENEMIES AND CASTLES */
		for(i=0;i<MAX_ENEMIES;i++){
			if (enemies[i].alive && (box_collision(&castles[0],&enemies[i]) || box_collision(&castles[1],&enemies[i]) || box_collision(&castles[2],&enemies[i]))){
				enemies[i].alive = 0;
				explosions[i].x = enemies[i].x;
				explosions[i].y = enemies[i].y;
				lives = 0;
				explosions[MAX_ENEMIES].x = player.x;
				explosions[MAX_ENEMIES].y = player.y;
				Mix_PlayChannel(EXPLODE_WAV,sounds[EXPLODE_WAV],0);
			}
		}

		/* ENEMY SHOTS AND CASTLES */
		for(i=0;i<MAX_ENEMY_SHOTS;i++){
			if (!enemy_shots[i].alive) continue;
			if (pixel_collision(&enemy_shots[i],&castles[0],1)){
				castle_hit(&castles[0],enemy_shots[i].x,enemy_shots[i].y,1);
				enemy_shots[i].alive=0;
				continue;
			} else if (pixel_collision(&enemy_shots[i],&castles[1],1)){
				castle_hit(&castles[1],enemy_shots[i].x,enemy_shots[i].y,1);
				enemy_shots[i].alive=0;
				continue;
			} else if (pixel_collision(&enemy_shots[i],&castles[2],1)){
				castle_hit(&castles[2],enemy_shots[i].x,enemy_shots[i].y,1);
				enemy_shots[i].alive=0;
				continue;
			}

			if (enemy_shots[i].y >= player.y){
				if (box_collision(&enemy_shots[i],&player)){
					player_hit();
					Mix_PlayChannel(EXPLODE_WAV,sounds[EXPLODE_WAV], 0);
					enemy_shots[i].alive = 0;
					break;
				}
				enemy_shots[i].alive = 0;
			}
		}
				






		/* Draw the enemies, shots, player, and explosions */
		for ( i=0; i<MAX_ENEMIES; i++ ) {
			if ( enemies[i].alive) {
				DrawObject(&enemies[i]);
			}
		}
		for ( i=0; i<MAX_SHOTS; ++i ) {
			if ( shots[i].alive ) {
				DrawObject(&shots[i]);
			}
		}
		for(i=0;i<MAX_ENEMY_SHOTS;i++){
			if (enemy_shots[i].alive){
				DrawObject(&enemy_shots[i]);
			}
		}

		for(i=0;i<3;i++){
			DrawObject(&castles[i]);
		}
		if (player.alive<3){
			if (player.alive==2){
				player.alive--;
				player.x = (SCREEN_WIDTH-player.image->w)/2;
			}
			DrawObject(&player);
		} else {
			player.alive--;
		}
		for ( i=0; i<MAX_ENEMIES+1; ++i ) {
			if ( explosions[i].alive ) {
				DrawObject(&explosions[i]);
			}
		}

		draw_points(0);

		for(i=0;i<lives;i++){
			SDL_Rect a;
			a.x=LIVES_OFFSET + (player.image->w + LIVES_SPACING)*i;
			a.y=screen->h - player.image->h - LIVES_HEIGHT;
			SDL_BlitSurface(player.image,NULL,screen,&a);
		}			

//		/* Loop the music */
//		if ( ! Mix_Playing(MUSIC_WAV) ) {
//			Mix_PlayChannel(MUSIC_WAV, sounds[MUSIC_WAV], 0);
//		}

		/* Check for keyboard abort */
		if ( keys[SDLK_ESCAPE] == SDL_PRESSED ) {
			player.alive = 0;
		}
		SDL_UpdateRect(screen,0,0,0,0);

	}

	/* Wait for the player to finish exploding */
//	while(Mix_Playing()){
//		wait();
//	}
//	Mix_HaltChannel(-1);

	return;
}


int main(int argc, char *argv[]){
	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO)<0){
		fprintf(stderr,"Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(2);
	}
	atexit(SDL_Quit);
	/* Open the audio device */
	if (Mix_OpenAudio(11025, AUDIO_U8, 1, 512)<0){
		fprintf(stderr,
		"Warning: Couldn't set 11025 Hz 8-bit audio\n- Reason: %s\n",SDL_GetError());
	}


	Uint32 key;
	SDL_Surface *wm_icon;

	wm_icon = SDL_LoadBMP(DATAFILE("wm_icon.bmp"));
	key = SDL_MapRGB(wm_icon->format, 255, 0, 255);
	SDL_SetColorKey(wm_icon, SDL_SRCCOLORKEY, key); 
	SDL_WM_SetIcon(wm_icon,NULL);

	/* Open the display device */
	screen = SDL_SetVideoMode(SCREEN_WIDTH,SCREEN_HEIGHT,32,SDL_HWSURFACE);
	if (screen==NULL){
		fprintf(stderr, "Couldn't set 640x480 video mode: %s\n",SDL_GetError());
		exit(2);
	}

	high_score_data_is_real = 0;
//	get_scores();
	if (load_data()){
		/* Initialize the random number generator */
		srand(time(NULL));

		SDL_WM_SetCaption("Space Resistors!","OMGOMGOMG");
		SDL_ShowCursor( SDL_DISABLE );

		player_scores[0].points = 0;
		player_scores[1].points = 0;

		current_player = 0;

		requests_quit=0;

		while(!requests_quit){
			/* Get some titular action going on */
			show_title_screen();

			SDL_Delay(200);

			if (current_player == 1){
				num_players = 2;
				current_player = 0;
			}

			if (requests_quit) break;

			/* Run the game */
			game();
			if (num_players==2){
				current_player = (current_player+1)%2;
				printf("switching to player %d\n",current_player+1);
				game();
			}
			end_screen();
		}

		/* Free the music and artwork */
		FreeData();
		Mix_CloseAudio();
	} else {
		printf("failed to init data\n");
	}

	exit(0);
}
