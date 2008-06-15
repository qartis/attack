#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include "SFont.h"

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#define	FRAMES_PER_SEC	30
#define SCREEN_WIDTH    374
#define SCREEN_HEIGHT   480
#define MAX_ENEMIES     ENEMY_ROWS * ENEMY_COLUMNS

#define LIVES		3
#define MAX_SHOTS	10
#define MAX_ENEMY_SHOTS 1
#define MAX_EXPLOSIONS MAX_ENEMIES+MAX_ENEMY_SHOTS+1+1

#define ENEMY_SPACING_WIDTH 33
#define ENEMY_SPACING_HEIGHT 25
#define LIVES_OFFSET	20
#define ENEMY_OFFSET    75
#define BOSS_HEIGHT     50
#define BOSS_SPEED      3
#define BOSS_TIMER_MIN  700
#define BOSS_TIMER_MAX  800
#define BOSS_SIDE_DELAY 50
#define LIVES_HEIGHT    1
#define PLAYER_HEIGHT   30
#define CASTLE_OFFSET   110
#define CASTLE_WIDTH    4*12
#define CASTLE_HEIGHT   3*12
#define SHOT_MAXHEIGHT  30
#define LIVES_SPACING   10
#define ENEMY_ROWS      5
#define ENEMY_COLUMNS   7

#define PLAYER_SPEED	3
#define SHOT_SPEED	10
#define ENEMY_SPEED	20
#define ENEMY_SHOT_SPEED 5
#define ENEMY_SHOT_FLICKER 3
#define ENEMY_SHOT_MIN_DELAY 25
#define ENEMY_SHOT_MAX_DELAY 75
#define PLAYER_DEATH_DELAY 40
#define ENEMY_MOVE_RATE 80
#define MOVE_RATE_DECREMENT 10
#define MINIMUM_MOVE_RATE 5
#define EXPLODE_TIME	15
#define SCREEN_PADDING 0
#define PADDING 10
#define PADDING_DECREMENT 2

#define HIGHSCORES_WIDTH 100
#define HIGHSCORES_STARTHEIGHT 160

enum {
	BLACK,
	BROWN,
	RED,
	ORANGE,
	YELLOW,
	GREEN,
	BLUE,
	VIOLET,
	GREY,
	WHITE,
	TRANSPARENT,
	NUM_COLOURS
};
	
typedef struct {
	int alive;
	int facing;
	int x, y;
	SDL_Surface *image;
	int colour;
    int type;
} object;

typedef struct {
    int x, y;
    int w, h;
    object bricks[12];
} castle;

typedef struct {
	int points;
	char name[3];
} score;

SFont_Font* font[7];

SDL_Surface *screen;
SDL_Surface *title_screen;
SDL_Surface *shot_images[10];
SDL_Surface *enemy_images[3][10][2];
SDL_Surface *enemy_shot_images[2][2];
SDL_Surface *castle_bits[5][4];
SDL_Surface *intro_frames[16];

object player;
object shots[MAX_SHOTS];
object enemy_shots[MAX_ENEMY_SHOTS];
object enemies[MAX_ENEMIES];
object explosions[MAX_EXPLOSIONS];//player, boss
castle castles[3];
object boss;

SDL_TimerID intro_timer;

int num_players;
int requests_quit;
int current_player; // 0 or 1
int points_lookup[11];
int high_score_data_is_real;
char url[128];
int flags; //video flags for fullscreen

score high_scores[10];
score player_scores[2];

Uint32 colours[10];

int fd;

enum {
	SHOT_WAV,
	EXPLODE_WAV,
    PAUSE_WAV,
	UFO_WAV,
	UFO_EXPLODE_WAV,
    NUM_WAVES
};

enum {
    MENU_SONG,
    DEAD_SONG,
    LEVEL1_SONG,
	INTRO_SONG,
    NUM_SONGS
};

enum {
    LEFT,
    UP,
    DOWN,
    RIGHT
};

Mix_Chunk *sounds[NUM_WAVES];
Mix_Music *songs[NUM_SONGS];
void   setpixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
Uint32 getpixel(SDL_Surface *surface, int x, int y);
void setfourpixels(object *castle, int x, int y);

SDL_Surface *tint(SDL_Surface *sprite, Uint32 to);

int can_shoot(int);
int dead_shot(void);
void player_hit(void);

int load_data(void);
void castle_hit(object *castle, int x, int y, int down);
int rand_between(int low, int high);
void wait(void);

int pixel_collision(object *sprite1, object *sprite2, int bottom);
int box_collision(object *sprite1, object *sprite2, int padding);

int find_enemy_who_can_shoot(void);
int need_reverse_enemies(void);

void draw_points(int disabled);
void show_title_screen(void);
void game(int,int);
void highscores();
void vidmode();
int open_resource(char *filename);
char *get_resource(char *resourcename, int *filesize);
SDL_Surface *load_bmp(char *filename);
Mix_Chunk *load_sound(char *filename);
Mix_Music *load_song(char *filename);
SDL_Surface *transparent(SDL_Surface*);
void mkcastle(castle*);
int castle_collision(object*,castle*);
void castle_block_hit(castle*,int);
void draw_castle(castle*);
void toggle_mute(void);
void __debug(const char*,int,const char *fmt,...) __attribute__((format(printf, 3, 4)));
#define DBG(code) code 
#define DEBUG(...) DBG(__debug(__FUNCTION__, __LINE__, __VA_ARGS__))
