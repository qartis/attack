#ifndef ATTACK_H
#define ATTACK_H ATTACK_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curl/curl.h>

#include "SDL.h"
#include "SFont.h"
#include "SDL_mixer.h"
#include "SDL_image.h"

#ifdef macintosh
#define DIR_SEP	":"
#define DIR_CUR ":"
#else
#define DIR_SEP	"/"
#define DIR_CUR	""
#endif
#define DATAFILE(X)	DIR_CUR "data" DIR_SEP X



#define	FRAMES_PER_SEC	60
#define SCREEN_WIDTH    374
#define SCREEN_HEIGHT   480
#define MAX_ENEMIES     ENEMY_ROWS * ENEMY_COLUMNS


#define LIVES		3
#define MAX_SHOTS	1
#define MAX_ENEMY_SHOTS 5

#define LIVES_OFFSET	20
#define ENEMY_OFFSET    50
#define LIVES_HEIGHT    1
#define PLAYER_HEIGHT   40
#define CASTLE_HEIGHT   50
#define SHOT_MAXHEIGHT  30
#define LIVES_SPACING   10
#define ENEMY_ROWS      5
#define ENEMY_COLUMNS   6

#define PLAYER_SPEED	1
#define SHOT_SPEED	2
#define ENEMY_SPEED	17
#define ENEMY_SHOT_SPEED 1
#define ENEMY_SHOT_FLICKER 12
#define PLAYER_DEATH_DELAY 100
#define ENEMY_MOVE_RATE 200
#define ENEMY_SHOT_CHANCE 500
#define CHOMP_SPEED     40
#define EXPLODE_TIME	15

typedef struct {
	int alive;
	int facing;
	int x, y;
	SDL_Surface *image;
	int colour;
} object;

typedef struct {
	int points;
	char name[3];
} score;

SFont_Font* font[7];

SDL_Surface *screen;
SDL_Surface *title_screen;
SDL_Surface *wm_icon;
SDL_Surface *shot_images[10];
SDL_Surface *enemy_images[10][2];
SDL_Surface *castle[3];
SDL_Surface *enemy_shot_images[2][2];
SDL_Surface *ic_image;

object player;
object shots[MAX_SHOTS];
object enemy_shots[MAX_ENEMY_SHOTS];
object enemies[MAX_ENEMIES];
object explosions[MAX_ENEMIES+1];
object castles[3];
object ic;

int enemy_direction;
int generic_counter;
int chomp_counter;
int chomp_frame;
int enemy_shot_counter;
int enemy_shot_frame;
int music_counter;
int music_speed;
int enemy_movement_counter;
int num_players;
int requests_quit;
int lives;
int current_player; // 0 or 1
int points_lookup[10];
int high_score_data_is_real;

score high_scores[10];
score player_scores[2];

Uint32 colours[10];

enum {
	MUSIC_WAV,
	SHOT_WAV,
	EXPLODE_WAV,
	NUM_WAVES
};

Mix_Chunk *sounds[NUM_WAVES];


void   setpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
Uint32 getpixel(SDL_Surface *surface, int x, int y);
void setfourpixels(object *castle, int x, int y);

SDL_Surface *LoadImage(char *datafile, int transparent);
SDL_Surface* tint(SDL_Surface *sprite, Uint32 to);

int can_shoot(void);
int dead_shot(void);
void player_hit(void);

int load_data(void);
void free_data(void);
void castle_hit(object *castle, int x, int y, int down);
int rand_between(int low, int high);
void draw(object *sprite);
void wait(void);

int pixel_collision(object *sprite1, object *sprite2, int bottom);
int box_collision(object *sprite1, object *sprite2);

int find_enemy_who_can_shoot(void);
int need_reverse_enemies(void);

size_t curl_write_data(void* buffer, size_t size, size_t nmemb, void* userp);

void draw_points(int disabled);
void show_title_screen(void);
void game(void);

#endif
