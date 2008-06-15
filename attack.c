#include "attack.h"
#include <sys/stat.h>
#include <dirent.h>

void toggle_mute(){
    Mix_VolumeMusic(Mix_VolumeMusic(-1)==0?128:0);
    Mix_Volume(-1, Mix_Volume(-1,-1)==0?128:0);
}

void musicFinished(){
	DEBUG("music finished");
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
}

int rand_between(int low, int high){
    int width = high - low + 1;
    return low + (int)((double)width * (rand() / ((double)RAND_MAX + 1.0)));
}

int can_shoot(int colour){
    int retval = -1;
    int i;
    for (i = 0; i < MAX_SHOTS; i++) {
        if (!shots[i].alive) {
            retval = i;
        } else if (shots[i].colour == colour)
            return -1;
    }
    return retval;
}

Uint32 hello(Uint32 interval, void *param){
	SDL_UserEvent event;
	event.type = SDL_USEREVENT;
	long code = (long)param;
	event.code = (int)code;
	DEBUG("just pushed an event with code %d",event.code);
	SDL_PushEvent((SDL_Event*)&event);
	return 0;
}

int intro_frame_delay[] = {5000,6000,5500,5500,5500,5500,5500,5500,5500,5500,5500,5500,5500,5500,5500,4500};
int NUM_INTRO_FRAMES = sizeof(intro_frame_delay)/sizeof(int);

void draw_intro_frame(long frame){
	DEBUG("drawing intro frame %ld",frame);
	if (frame < NUM_INTRO_FRAMES){
		intro_timer = SDL_AddTimer(intro_frame_delay[frame],hello,(void*)frame+1);
		if (frame == 0){
			SDL_FillRect (screen, NULL, colours[0]);
			char *text = "BAMCODE PRESENTS";
			SFont_Write(screen, font[0], (SCREEN_WIDTH - SFont_TextWidth(font[2], text))/2, 230, text);
		} else {
			SDL_BlitSurface(intro_frames[frame],NULL,screen,NULL);
		}
		SDL_UpdateRect(screen,0,0,0,0);
	} else {
		DEBUG("done");
		SDL_KeyboardEvent event;
		event.type = SDL_KEYDOWN;
		SDL_PushEvent((SDL_Event*)&event);
	}
}

void play_intro(){
	SDL_Event event;
	Uint8 *keys;
	DEBUG("started play_intro()");
	Mix_PlayMusic(songs[INTRO_SONG], 0);
	draw_intro_frame(0);
	for(;;){
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            requests_quit = 1;
			goto ret2;
        }
		if (event.type == SDL_USEREVENT){
			draw_intro_frame(event.user.code);
		}
        keys = SDL_GetKeyState(NULL);
        if (keys[SDLK_ESCAPE] == SDL_PRESSED || keys[SDLK_q] == SDL_PRESSED) {
            requests_quit = 1;
            goto ret2;
        }
		if (keys[SDLK_f] == SDL_PRESSED){
			vidmode();
			continue;
		}			
		if (event.type == SDL_KEYDOWN){
			DEBUG("person is skipping the animation");
			goto ret;
		}
	}
		
ret:
	Mix_FadeOutMusic(500);
	while(Mix_FadingMusic() == MIX_FADING_OUT){
		SDL_Delay(50);
	}
	SDL_Delay(1000);
ret2:
	SDL_RemoveTimer(intro_timer);
}

inline void mkcastle(castle *new){
    int i;
    for(i=0;i<12;i++){
        new->bricks[i].alive = 4;
        new->bricks[i].colour = 2;
        new->bricks[i].image = castle_bits[2][0];
        new->bricks[i].x = (i%4)*castle_bits[0][0]->w  + new->x;
        new->bricks[i].y = ((i-(i%4))/4)*castle_bits[0][0]->h + new->y;
    }
    new->bricks[0].colour = 0;
    new->bricks[0].image = castle_bits[0][0];
    new->bricks[3].colour = 1;
    new->bricks[3].image = castle_bits[1][0];
    new->bricks[5].colour = 3;
    new->bricks[5].image = castle_bits[3][0];
    new->bricks[6].colour = 4;
    new->bricks[6].image = castle_bits[4][0];
    new->bricks[9].alive = 0;
    new->bricks[10].alive = 0;
}

int castle_collision(object *obj, castle *cast){
    int i;
    for(i=0;i<12;i++){
        if (cast->bricks[i].alive > 0 && box_collision(obj,&(cast->bricks[i]),0)){
            return i;
        }
    }
    return -1;
}

void castle_block_hit(castle *cast, int brick){
    cast->bricks[brick].alive--;
    if (cast->bricks[brick].alive > 0){
        cast->bricks[brick].image = castle_bits[cast->bricks[brick].colour][4-cast->bricks[brick].alive];
    }
}


void draw_object(object * sprite){
    if (sprite->alive < 1) return;
    SDL_Rect a;
    a.x = sprite->x;
    a.y = sprite->y;
    SDL_BlitSurface(sprite->image, NULL, screen, &a);
}

void draw_castle(castle *cast){
    int i;
    for(i=0;i<12;i++){
        draw_object(&(cast->bricks[i]));
    }
}

int box_collision(object * sprite1, object * sprite2, int padding){
    if ((sprite1->y >= (sprite2->y + sprite2->image->h)) ||
        (sprite1->x >= (sprite2->x + sprite2->image->w + padding)) ||
        (sprite2->y >= (sprite1->y + sprite1->image->h)) ||
        (sprite2->x >= (sprite1->x + sprite1->image->w + padding))) {
        return 0;
    }
    return 1;
}

int dead_shot(){
    int i;
    for (i = 0; i < MAX_ENEMY_SHOTS; i++) {
        if (!enemy_shots[i].alive)
            return i;
    }
    return -1;
}

int find_enemy_who_can_shoot(){
    int i,j;
    int available_enemies[ENEMY_COLUMNS];
    int num_available = 0;

    for (i = 0; i < ENEMY_COLUMNS; i++) {
        for (j = ENEMY_ROWS - 1; j > -1; j--) {
            if (enemies[j * ENEMY_COLUMNS + i].alive) {
                available_enemies[num_available++] = j * ENEMY_COLUMNS + i;
                break;
            }
        }
    }

    if (num_available < 1)
        return -1;
    return available_enemies[rand_between(0, num_available - 1)];
}

void wait(void){
    static Uint32   next_tick = 0;
    Uint32          this_tick;
    /*
     * Wait for the next frame 
     */
    this_tick = SDL_GetTicks();
    if (this_tick < next_tick)
        SDL_Delay(next_tick - this_tick);
    next_tick = this_tick + (1000 / FRAMES_PER_SEC);
}

void draw_points(int disabled){
    SFont_Font *player1_name;
    SFont_Font *player1_points;
    SFont_Font *player2_name;
    SFont_Font *player2_points;

    if (disabled) {
        player1_name = font[6];
        player1_points = font[6];
        player2_name = font[6];
        player2_points = font[6];
    } else {
        if (current_player == 1) {
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

    SFont_Write(screen, player1_points, 6, 5, "SCORE-1");

    char            buf[7];
    sprintf(buf, "%.6d", player_scores[0].points);
    SFont_Write(screen, player1_points, 9, 17, buf);

    if (num_players == 2) {
        SFont_Write(screen, player2_points,
                    SCREEN_WIDTH - 6 - SFont_TextWidth(font[2], "SCORE-2"),
                    5, "SCORE-2");
        sprintf(buf, "%.6d", player_scores[1].points);
        SFont_Write(screen, player2_points,
                    SCREEN_WIDTH - 9 - SFont_TextWidth(font[2], buf), 17,
                    buf);
    } else {
        SDL_Rect box;
        box.x = SCREEN_WIDTH - 6 - SFont_TextWidth(font[2], "SCORE-2");
        box.y = 5;
        box.w = SFont_TextWidth(font[0],"SCORE-2");
        box.h = (SFont_TextHeight(font[0])+5)*2;
        SDL_FillRect (screen, &box, colours[0]);
    }

    SFont_Write(screen, font[3],
                (SCREEN_WIDTH - SFont_TextWidth(font[3], "TAITO")) / 2, 6,
                "TAITO");
}

void show_title_screen(void){
    SDL_Event event;
    Uint8 *keys;
    int down_was_pressed,up_was_pressed,mute_was_pressed=0;
    num_players = 1;
    current_player = 0;

music:
	if (requests_quit) return;
	Mix_PlayMusic(songs[MENU_SONG], 0); //race condition: Mix_HookMusicFinished must come after PlayMusic or it triggers itself
	Mix_HookMusicFinished(musicFinished);
refresh:
    down_was_pressed = 0;
    up_was_pressed = 1;
    SDL_BlitSurface(title_screen, NULL, screen, NULL);
    draw_points(1);
    SDL_UpdateRect(screen,0,0,0,0);
    for (;;) {
        if (down_was_pressed || up_was_pressed){
            if (current_player == 0) {
                SFont_Write(screen, font[0],
                            (screen->w -
                             SFont_TextWidth(font[0], "1 PLAYER")) / 2, 340,
                            "1 PLAYER");
                SFont_Write(screen, font[1],
                            (screen->w -
                             SFont_TextWidth(font[1], "2 PLAYER")) / 2, 360,
                            "2 PLAYERS");
            } else {
                SFont_Write(screen, font[1],
                            (screen->w -
                             SFont_TextWidth(font[0], "1 PLAYER")) / 2, 340,
                            "1 PLAYER");
                SFont_Write(screen, font[0],
                            (screen->w -
                             SFont_TextWidth(font[1], "2 PLAYER")) / 2, 360,
                            "2 PLAYERS");
            }
            draw_points(1);
            SDL_UpdateRect(screen, 0, 0, SCREEN_WIDTH, 30);
            SDL_UpdateRect(screen, 0, 340, SCREEN_WIDTH, 30);
        }

        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            requests_quit = 1;
            return;
        }
		if (event.type == SDL_USEREVENT){
			Mix_HookMusicFinished(NULL);
			DEBUG("destroyed hook fdas");
			play_intro();
			goto music;
		}
        keys = SDL_GetKeyState(NULL);
        if (keys[SDLK_ESCAPE] == SDL_PRESSED || keys[SDLK_q] == SDL_PRESSED) {
            requests_quit = 1;
            return;
        }
		DBG(if (keys[SDLK_i] == SDL_PRESSED){
			DEBUG("manually starting intro");
			Mix_HookMusicFinished(NULL);
			play_intro();
			goto music;
		})
        if (keys[SDLK_DOWN] == SDL_PRESSED) {
            if (!down_was_pressed){
                current_player = 1;
                num_players = 2;
                down_was_pressed = 1;
            }
        } else {
            down_was_pressed = 0;
        }
        if (keys[SDLK_UP] == SDL_PRESSED) {
            if (!up_was_pressed){
                current_player = 0;
                num_players = 1;
                up_was_pressed = 1;
            }
        } else {
            up_was_pressed = 0;
        }
        if (keys[SDLK_1] == SDL_PRESSED) {
            current_player = 0;
            return;
        } else if (keys[SDLK_2] == SDL_PRESSED) {
            current_player = 1;
            return;
        } else if (keys[SDLK_RETURN] == SDL_PRESSED){
            return;
        } else if (keys[SDLK_f] == SDL_PRESSED){
            vidmode();
            goto refresh;
        }
        if (keys[SDLK_m] == SDL_PRESSED){
            if (!mute_was_pressed){
                toggle_mute();
                mute_was_pressed = 1;
            }
        } else {
            mute_was_pressed = 0;
        }
    }
}

void player_go(int i){
    SDL_FillRect (screen, NULL, colours[0]);
    char *text = strdup("PLAYER n GO");
    text[strlen("PLAYER ")] = (i+1)+'0';
    SFont_Write(screen,font[1],(SCREEN_WIDTH-SFont_TextWidth(font[0],text))/2,(SCREEN_HEIGHT-SFont_TextHeight(font[0]))/2,text);
    SDL_UpdateRect(screen,0,0,0,0);
    SDL_Delay(1000);
}

void get_name(int i){
    SDL_Event event;
    Uint8 *keys;

    int was_pressed[4] = {0,0,0,0};
    int current_letter = 0;
    player_scores[i].name[0] = 'A';
    player_scores[i].name[1] = 'A';
    player_scores[i].name[2] = 'A';

    char score_text[15];
    char name_text[10];
    char player_text[] = "PLAYER X";
    player_text[strlen("PLAYER ")] = i + '1';

    sprintf(score_text, "SCORE: %d", player_scores[i].points);

    for (;;) {
        SDL_FillRect(screen, NULL, colours[0]);
        SFont_Write(screen, font[0],
                    (screen->w -
                     SFont_TextWidth(font[0], "GAME OVER")) / 2, 150,
                    "GAME OVER");
        sprintf(name_text, "NAME: %c%c%c", player_scores[i].name[0],
                player_scores[i].name[1], player_scores[i].name[2]);

        SFont_Write(screen, font[0],
                    (screen->w -
                     SFont_TextWidth(font[0], player_text)) / 2, 185,
                    player_text);
        SFont_Write(screen, font[0],
                    (screen->w - SFont_TextWidth(font[0], score_text)) / 2,
                    200, score_text);
        SFont_Write(screen, font[0],
                    (screen->w -
                     SFont_TextWidth(font[0], "NAME: AAA")) / 2, 215,
                    name_text);
        SFont_Write(screen, font[0],
                    (screen->w + SFont_TextWidth(font[0], name_text)) / 2 -
                    SFont_TextWidth(font[0],
                                    "A") * ((2 - current_letter) + 1) - 1,
                    217, "_");

        SDL_UpdateRect(screen, 0, 0, 0, 0);
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT) {
            requests_quit = 1;
            return;
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym >= 'a'
                && event.key.keysym.sym <= 'z') {
                player_scores[i].name[current_letter++] =
                    event.key.keysym.sym - 'a' + 'A';
            }
            if (current_letter == 3)
                return;
        }
        keys = SDL_GetKeyState(NULL);
        if (keys[SDLK_ESCAPE] == SDL_PRESSED) {
            requests_quit = 1;
            return;
        } else if (keys[SDLK_RETURN] == SDL_PRESSED) {
            return;
        }
        if (keys[SDLK_LEFT] == SDL_PRESSED || keys[SDLK_BACKSPACE] == SDL_PRESSED) {
            if (!was_pressed[LEFT]) {
                current_letter -= 1;
                if (current_letter < 0)
                    current_letter = 0;
                was_pressed[LEFT] = 1;
            }
        } else {
            was_pressed[LEFT] = 0;
        }

        if (keys[SDLK_RIGHT] == SDL_PRESSED) {
            if (!was_pressed[RIGHT]) {
                current_letter += 1;
                if (current_letter > 2)
                    current_letter = 2;
                was_pressed[RIGHT] = 1;
            }
        } else {
            was_pressed[RIGHT] = 0;
        }
        if (keys[SDLK_UP] == SDL_PRESSED) {
            if (!was_pressed[UP]) {
                player_scores[i].name[current_letter]--;
                if (player_scores[i].name[current_letter] < 'A')
                    player_scores[i].name[current_letter] = 'A';
                was_pressed[UP] = 1;
            }
        } else {
            was_pressed[UP] = 0;
        }

        if (keys[SDLK_DOWN] == SDL_PRESSED) {
            if (!was_pressed[DOWN]) {
                player_scores[i].name[current_letter]++;
                if (player_scores[i].name[current_letter] > 'Z')
                    player_scores[i].name[current_letter] = 'Z';
                was_pressed[DOWN] = 1;
            }
        } else {
            was_pressed[DOWN] = 0;
        }
    }
}

void toggle_pause(int *is_paused){
    if (*is_paused){
        Mix_HaltChannel(PAUSE_WAV);
        Mix_ResumeMusic();
        *is_paused = 0;
    } else {
        Mix_PauseMusic();
        Mix_PlayChannel(PAUSE_WAV,sounds[PAUSE_WAV],-1);
        *is_paused = 1;
    }
}

void game(int enemy_move_rate, int padding){
    int i,j,k;
    Uint8 *keys;
    SDL_Event event;

    player.alive = 1;
    player.x = (screen->w - player.image->w) / 2;
    player.y = (screen->h - player.image->h) - 1 - PLAYER_HEIGHT;
    player.facing = 0;

#ifndef RAPIDFIRE
    int waspressed[10];
    for(i=0;i<10;i++) waspressed[i] = 0;
#endif

    for (i = 0; i < MAX_SHOTS; i++) {
        shots[i].alive = 0;
    }

    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        explosions[i].alive = 0;
    }

    for (i = 0; i < ENEMY_COLUMNS; i++) {
        enemies[0 * ENEMY_COLUMNS + i].colour = rand_between(0, 1);
        enemies[0 * ENEMY_COLUMNS + i].type = 0;
        enemies[1 * ENEMY_COLUMNS + i].colour = rand_between(2, 5);
        enemies[2 * ENEMY_COLUMNS + i].colour = rand_between(2, 5);
        enemies[1 * ENEMY_COLUMNS + i].type = 1;
        enemies[2 * ENEMY_COLUMNS + i].type = 1;
        enemies[3 * ENEMY_COLUMNS + i].colour = rand_between(6, 9);
        enemies[4 * ENEMY_COLUMNS + i].colour = rand_between(6, 9);
        enemies[3 * ENEMY_COLUMNS + i].type = 2;
        enemies[4 * ENEMY_COLUMNS + i].type = 2;
    }

    for (i = 0; i < ENEMY_ROWS; i++) {
        for (j = 0; j < ENEMY_COLUMNS; j++) {
            int n = i * ENEMY_COLUMNS + j;
            enemies[n].alive = 1;
            enemies[n].image = enemy_images[enemies[n].type][enemies[n].colour][0];
            enemies[n].y = ENEMY_SPACING_HEIGHT * i + ENEMY_OFFSET;
			if (i == 0) enemies[n].y -= 2;
			if (i > 2) enemies[n].y += 2;
            enemies[n].x = ENEMY_SPACING_WIDTH * j;
        }
    }

    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
        enemy_shots[i].alive = 0;

    int enemy_direction = 1;
    int chomp_counter = 0;
    int chomp_frame = 0;
    int enemy_shot_counter = 0;
    int enemy_shot_frame = 0;
    int enemy_movement_counter = 0;
    int lives = LIVES;
    int mute_was_pressed = 0;
    int is_paused=0;
    int pause_was_pressed=0;
	int next_shot_timer=0;
	int boss_timer;

    i = CASTLE_WIDTH/2;
    castles[0].x = 1 * (SCREEN_WIDTH / 4) - i;
    castles[1].x = 2 * (SCREEN_WIDTH / 4) - i;
    castles[2].x = 3 * (SCREEN_WIDTH / 4) - i;
    castles[0].y = SCREEN_HEIGHT - CASTLE_OFFSET;
    castles[1].y = castles[0].y;
    castles[2].y = castles[0].y;

	boss.x = -boss.image->w;
	boss.y = BOSS_HEIGHT;
	boss.alive = 0;
	boss.facing = 1;
	boss_timer = rand_between(BOSS_TIMER_MIN,BOSS_TIMER_MAX);
	Mix_HaltChannel(UFO_WAV);// if there was a ufo onscreen when player won last round

    for (i = 0; i < 3; i++) mkcastle(&castles[i]);

    while (lives > 0 && !requests_quit) {
        wait();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                requests_quit = 1;
                continue;
            }
        }
        keys = SDL_GetKeyState(NULL);

        if (keys[SDLK_q] == SDL_PRESSED
            || keys[SDLK_ESCAPE] == SDL_PRESSED) {
            requests_quit = 1;
            continue;
        }
        if (keys[SDLK_m] == SDL_PRESSED){
            if (!mute_was_pressed){
                toggle_mute();
                mute_was_pressed = 1;
            }
        } else {
            mute_was_pressed = 0;
        }
        if (keys[SDLK_SPACE] == SDL_PRESSED || keys[SDLK_p] == SDL_PRESSED || keys[SDLK_PAUSE] == SDL_PRESSED){
            if (!pause_was_pressed){
                toggle_pause(&is_paused);
                pause_was_pressed = 1;
            }
        } else {
            pause_was_pressed = 0;
        }
        if (is_paused) continue;
        SDL_FillRect(screen, NULL, colours[0]);

        /*
         * Decrement the lifetime of the explosions 
         */
        for (i = 0; i < MAX_EXPLOSIONS; i++) {
            if (explosions[i].alive > 0)
                explosions[i].alive--;
        }

        /*
         * Create new shots 
         */
        for (i = 0; i < 10; i++) {
            if (keys[SDLK_0 + i] == SDL_PRESSED || keys[SDLK_KP0 + i] == SDL_PRESSED) {
#ifndef RAPIDFIRE
                if (!waspressed[i]){
#endif
                int j;
                if (-1 < (j = can_shoot(i))) {
                    shots[j].alive = 1;
                    shots[j].colour = i;
                    shots[j].image = shot_images[i];
                    shots[j].x = player.x + (player.image->w - shots[j].image->w) / 2;
                    shots[j].y = player.y - shots[j].image->h;
                    Mix_PlayChannel(SHOT_WAV, sounds[SHOT_WAV], 0);
                    break;
                }
#ifndef RAPIDFIRE
                waspressed[i] = 1;
                }
            } else {
                waspressed[i] = 0;
#endif
            }

        }
        if (keys[SDLK_f] == SDL_PRESSED){
            vidmode();
        }
        player.facing = 0;
        if (keys[SDLK_RIGHT] == SDL_PRESSED)
            player.facing++;
        if (keys[SDLK_LEFT] == SDL_PRESSED)
            player.facing--;

		if (player.alive){
			player.x += player.facing * PLAYER_SPEED;

			if (player.x < 0){
				player.x = 0;
			} else if (player.x >= (screen->w - player.image->w)) {
				player.x = (screen->w - player.image->w) - 1;
			}
		}

        if (chomp_counter > enemy_move_rate/4) {
            chomp_frame = (chomp_frame + 1) % 2;
            chomp_counter = 0;
            for (i = 0; i < MAX_ENEMIES; i++) {
                enemies[i].image = enemy_images[enemies[i].type][enemies[i].colour][chomp_frame];
            }
        } else {
            chomp_counter++;
        }

		if (boss.alive){
			boss.x += BOSS_SPEED*boss.facing;
			if (boss.x > SCREEN_WIDTH || boss.x < -boss.image->w){
				boss.alive = 0;
				boss.facing = -boss.facing;
				if (boss.x > SCREEN_WIDTH) boss_timer = BOSS_SIDE_DELAY;
				else Mix_HaltChannel(UFO_WAV);
			}
		} else if (boss_timer < 1 DBG(|| keys[SDLK_b] == SDL_PRESSED)){
			boss.alive = 1;
			Mix_PlayChannel(UFO_WAV,sounds[UFO_WAV],-1);
			boss_timer = rand_between(BOSS_TIMER_MIN,BOSS_TIMER_MAX);
		} else {
			boss_timer--;
		}

        /*
         * Detect new enemy shots 
         */
		if (next_shot_timer < 1 || keys[SDLK_e] == SDL_PRESSED){
			next_shot_timer = rand_between(ENEMY_SHOT_MIN_DELAY,ENEMY_SHOT_MAX_DELAY);
            if (((i = dead_shot()) != -1) && ((j = find_enemy_who_can_shoot()) != -1)){
                enemy_shots[i].colour = rand_between(0, 1);
                enemy_shots[i].image = enemy_shot_images[enemy_shots[i].colour][enemy_shot_frame];
                enemy_shots[i].alive = enemy_shots[i].colour + 1;
                enemy_shots[i].x = enemies[j].x + (enemies[j].image->w / 2);
                enemy_shots[i].y = enemies[j].y + enemies[j].image->h - 8;
                Mix_PlayChannel(SHOT_WAV, sounds[SHOT_WAV], 0);
            }
        } else {
			next_shot_timer--;
		}

        /*
         * Move the enemies' shots 
         */
        if (enemy_shot_counter > ENEMY_SHOT_FLICKER) {
            enemy_shot_frame = (enemy_shot_frame + 1) % 2;
            enemy_shot_counter = 0;
        } else {
            enemy_shot_counter++;
        }

        for (i = 0; i < MAX_ENEMY_SHOTS; i++){
            if (enemy_shots[i].alive) {
                enemy_shots[i].y += ENEMY_SHOT_SPEED;
                enemy_shots[i].image =
                    enemy_shot_images[enemy_shots[i].
                                      colour][enemy_shot_frame];
            }
        }


        /*
         * Move the enemies 
         */
        if (enemy_movement_counter > enemy_move_rate) {
            enemy_movement_counter = 0;

            int i, need_reverse = 0;
            for (i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].alive && (enemies[i].x + (enemy_direction * ENEMY_SPEED) <
                    SCREEN_PADDING
                    || (enemies[i].x + (enemy_direction * ENEMY_SPEED)) >=
                    screen->w - enemies[i].image->w - SCREEN_PADDING)) {
                    need_reverse = 1;
                    break;
                }
            }

            if (need_reverse) {
                enemy_direction = -enemy_direction;
                for (i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].alive)
                        enemies[i].y += enemies[0].image->h;
                }
            } else {
                for (i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].alive)
                        enemies[i].x += enemy_direction * ENEMY_SPEED;
                }
            }
        } else {
            enemy_movement_counter++;
        }

        /*
         * Move the players' shots 
         */
        for (i = 0; i < MAX_SHOTS; ++i) {
            if (shots[i].alive) {
                shots[i].y -= SHOT_SPEED;
                if (shots[i].y < SHOT_MAXHEIGHT) shots[i].alive = 0;
            }
        }

        /*
         * shots and aliens 
         */
        for (j = 0; j < MAX_SHOTS; ++j) {
            for (i = 0; i < MAX_ENEMIES; ++i) {
                if (shots[j].alive && enemies[i].alive
                    && shots[j].colour == enemies[i].colour
                    && box_collision(&shots[j], &enemies[i],padding)) {
                    enemies[i].alive = 0;
                    explosions[i].x = enemies[i].x;
                    explosions[i].y = enemies[i].y;
                    explosions[i].alive = EXPLODE_TIME;
					explosions[i].image = tint(explosions[i].image,enemies[i].colour==BLACK?colours[GREY]:colours[enemies[i].colour]);
                    Mix_PlayChannel(EXPLODE_WAV, sounds[EXPLODE_WAV], 0);
                    shots[j].alive = 0;
                    player_scores[current_player].points += points_lookup[enemies[i].colour];
					int remaining_enemies = 0;
                    for (i = 0; i < MAX_ENEMIES; i++) {
                        if (enemies[i].alive) remaining_enemies++;
                    }
					if (remaining_enemies%ENEMY_COLUMNS==0 || remaining_enemies<ENEMY_COLUMNS){
						enemy_move_rate -= MOVE_RATE_DECREMENT;
						enemy_move_rate = MAX(enemy_move_rate,MINIMUM_MOVE_RATE);
					}
					if (remaining_enemies > 0) goto jmp;
                    return;
                }
            }
            jmp:
            if (!shots[j].alive) continue;
            for(i=0;i<3;i++){
                if (-1 < (k = castle_collision(&shots[j],&castles[i]))){
                    shots[j].alive = 0;
                    castle_block_hit(&castles[i],k);
                    break;
                }
            }
			if (!shots[j].alive) continue;
			if (box_collision(&shots[j],&boss,padding)){
				boss.alive = 0;
				boss.facing = 1;
				boss_timer = rand_between(BOSS_TIMER_MIN,BOSS_TIMER_MAX);
				explosions[MAX_ENEMIES+1].x = boss.x+(boss.image->w-explosions[0].image->w)/2;
				explosions[MAX_ENEMIES+1].y = boss.y;
				explosions[MAX_ENEMIES+1].alive = EXPLODE_TIME*4;
				explosions[MAX_ENEMIES+1].image = tint(explosions[MAX_ENEMIES+1].image,colours[RED]);
				boss.x = -boss.image->w;
				Mix_PlayChannel(UFO_EXPLODE_WAV, sounds[UFO_EXPLODE_WAV], 0);
				Mix_HaltChannel(UFO_WAV);
				shots[j].alive = 0;
				player_scores[current_player].points += points_lookup[10];
			}
			if (!shots[j].alive) continue;
			for(i=0;i<MAX_ENEMY_SHOTS;i++){
				if (enemy_shots[i].alive && box_collision(&shots[j],&enemy_shots[i],padding)){
					shots[j].alive = 0;
					enemy_shots[i].alive--;
					Mix_PlayChannel(EXPLODE_WAV, sounds[EXPLODE_WAV], 0);
					if (enemy_shots[i].alive < 1){
						explosions[MAX_ENEMIES+1+i].x = enemy_shots[i].x+(enemy_shots[i].image->w-explosions[MAX_ENEMIES+1+i].image->w)/2;
						explosions[MAX_ENEMIES+1+i].y = enemy_shots[i].y+(enemy_shots[i].image->h-explosions[MAX_ENEMIES+1+i].image->h)/2;
						explosions[MAX_ENEMIES+1+i].alive = EXPLODE_TIME;
						enemy_shots[i].alive = 0;
					}
					break;
				}
			}
		}

	/*
         * ENEMIES AND PLAYER 
         */
        for (i = 0; i < MAX_ENEMIES; ++i) {
            if (enemies[i].alive && box_collision(&player, &enemies[i],0)) {
                enemies[i].alive = 0;
                explosions[i].x = enemies[i].x;
                explosions[i].y = enemies[i].y;
                explosions[i].alive = EXPLODE_TIME;
                lives = 0;
                player.alive = 0;
                explosions[MAX_ENEMIES].x = player.x;
                explosions[MAX_ENEMIES].y = player.y;
                explosions[MAX_ENEMIES].alive = EXPLODE_TIME;
                Mix_PlayChannel(EXPLODE_WAV, sounds[EXPLODE_WAV], 0);
            }
        }

        /*
         * ENEMIES AND CASTLES 
         */
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            for(j=0;j<3;j++){
                if (-1 < (k = castle_collision(&enemies[i],&castles[j]))){
                    castles[j].bricks[k].alive = 0;
                    break;
                }
            }
        }

        /*
         * ENEMY SHOTS AND CASTLES 
         */
        for (i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemy_shots[i].alive) continue;
            for(j=0;j<3;j++){
                if (-1 < (k = castle_collision(&enemy_shots[i],&castles[j]))){
                    castle_block_hit(&castles[j],k);
                    enemy_shots[i].alive = 0;
                    break;
                }
            }

            if (enemy_shots[i].y > player.y + 5) {
                if (box_collision(&enemy_shots[i], &player,0)) {
                    lives--;
                    player.alive = PLAYER_DEATH_DELAY;
                    Mix_PlayChannel(EXPLODE_WAV, sounds[EXPLODE_WAV], 0);
                    enemy_shots[i].alive = 0;
                    explosions[MAX_ENEMIES].x = player.x - ((explosions[MAX_ENEMIES].image->w - player.image->w) / 2);
                    explosions[MAX_ENEMIES].y = player.y - ((explosions[MAX_ENEMIES].image->h - player.image->h) / 2);
                    explosions[MAX_ENEMIES].alive = EXPLODE_TIME;
                    if (lives == 0) player.alive = 0;
                    break;
                }
                enemy_shots[i].alive = 0;
            }
        }

        for(i=0;i<3;i++){
            draw_castle(&castles[i]);
        }
        for (i = 0; i < MAX_ENEMIES; i++) {
            draw_object(&enemies[i]);
        }
        for (i = 0; i < MAX_SHOTS; ++i) {
            draw_object(&shots[i]);
        }
        for (i = 0; i < MAX_ENEMY_SHOTS; i++) {
            draw_object(&enemy_shots[i]);
        }
		draw_object(&boss);

        if (player.alive < 3) {
            if (player.alive == 2) {
                player.alive--;
                player.x = (SCREEN_WIDTH - player.image->w) / 2;
            }
            draw_object(&player);
        } else {
            player.alive--;
        }
        for (i = 0; i < MAX_EXPLOSIONS; ++i) {
            draw_object(&explosions[i]);
        }

        draw_points(0);

        for (i = 0; i < lives; i++) {
            SDL_Rect a;
            a.x = LIVES_OFFSET + (player.image->w + LIVES_SPACING) * i;
            a.y = screen->h - player.image->h - LIVES_HEIGHT;
            SDL_BlitSurface(player.image, NULL, screen, &a);
        }

        SDL_UpdateRect(screen, 0, 0, 0, 0);
    }
    return;
}

void vidmode(void){
    if (NULL == (screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, flags))) {
        fprintf(stderr, "Couldn't set 640x480 video mode: %s\n",SDL_GetError());
        exit(EXIT_FAILURE);
    }
    flags ^= SDL_FULLSCREEN;
}

int main(int argc, char *argv[]){
	int i;
    if (1 > (fd = open_resource(argv[0]))){
        printf("fatal error\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Initialize the SDL library 
     */
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "Couldn't initialize sdl: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 1, 1024) < 0) {
        fprintf(stderr, "Failed to set audio mode: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    /*
     * Open the display device 
     */
    flags = SDL_SWSURFACE;
    if (argc > 1 && !strncmp(argv[1],"-fs",strlen("-fs"))) flags ^= SDL_FULLSCREEN;
	/*
	for(i=0;i<NUM_SONGS;i++) songs[i] = NULL;
	for(i=1;i<argc;i++){
		struct stat buf;
		stat(argv[i],&buf);
		if (S_ISDIR(buf.st_mode)){
			DIR *dir = opendir(argv[i]);
			struct dirent *ent;
			while(NULL != (ent = readdir(dir))){
				if (ent->d_name[0] == '.') continue;
				char *p = strrchr(ent->d_name,'.');
				if (p) p++;
				else continue;
				if (!strncasecmp(p,"mp3",3) || !strncasecmp(p,"ogg",3)){
					if (!strncasecmp(ent->d_name,"menu.",strlen("menu."))){
						songs[MENU_SONG] = load_song_file(argv[i],ent->d_name);
						printf("found custom menu song %s\n",ent->d_name);
					} else if (!strncasecmp(ent->d_name,"end.",strlen("end."))){
						songs[DEAD_SONG] = load_song_file(argv[i],ent->d_name);
						printf("found custom end song %s\n",ent->d_name);
					} else {
						printf("found custom play song %s\n",ent->d_name);
						todo
					}
				}
			}
		}
	}
	*/	
    vidmode();
    SDL_ShowCursor (SDL_DISABLE);

    high_score_data_is_real = 0;
    if (load_data()) {
        fprintf(stderr, "Failed to load data files\n");
        exit(EXIT_FAILURE);
    }

	Mix_Volume(UFO_WAV,MIX_MAX_VOLUME/4);
    SDL_Surface *icon = transparent(load_bmp("icon.bmp"));
    SDL_WM_SetIcon(icon, NULL);


    SDL_WM_SetCaption("Space Impeders!", "OMGOMGOMG");

    srand(time(NULL));
    requests_quit = 0;

	url[0] = '\0';
    while (!requests_quit) {
        current_player = 0;
        player_scores[0].points = 0;
        player_scores[1].points = 0;

        show_title_screen();
        if (requests_quit) break;
        Mix_FadeOutMusic(500);

        SDL_Delay(500);

        if (current_player == 1) {
            num_players = 2;
        } else {
            num_players = 1;
        }

        current_player = 0;

        /*
         * Run the game 
         */
        Mix_PlayMusic(songs[LEVEL1_SONG], -1);
		for(current_player=0;current_player<num_players&&!requests_quit;current_player++){
            player_go(current_player);
			int enemy_move_rate = ENEMY_MOVE_RATE, padding = PADDING;
			do {
				game(enemy_move_rate,padding);
				enemy_move_rate -= MOVE_RATE_DECREMENT;
				enemy_move_rate = MAX(enemy_move_rate,MINIMUM_MOVE_RATE);
				padding -= PADDING_DECREMENT;
				padding = MAX(padding,0);
			} while (!requests_quit && player.alive);
        }
        Mix_HaltMusic();
        if (requests_quit) break;
        SDL_Delay(1000);
        Mix_PlayMusic(songs[DEAD_SONG],-1);
        for (i = 0; i < num_players; i++) {
            get_name(i);
        }
        highscores();
        Mix_FadeOutMusic(400);
        SDL_Delay(400);
    }

    Mix_CloseAudio();

    exit(EXIT_SUCCESS);
}
