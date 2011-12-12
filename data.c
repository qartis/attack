#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <bzlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>

#include "sfont.h"
#include "attack.h"

#include "data_obj/fred.h"
#include "data_obj/title_screen.h"
#include "data_obj/blam.h"
#include "data_obj/icon.h"
#include "data_obj/font_bluish.h"
#include "data_obj/font_blue.h"
#include "data_obj/font_yellow.h"
#include "data_obj/font_white.h"
#include "data_obj/font_red.h"
#include "data_obj/font_green.h"
#include "data_obj/font_grey.h"
#include "data_obj/menu.h"
#include "data_obj/shot.h"
#include "data_obj/enemy_0_0.h"
#include "data_obj/enemy_0_1.h"
#include "data_obj/enemy_1_0.h"
#include "data_obj/enemy_1_1.h"
#include "data_obj/enemy_2_0.h"
#include "data_obj/enemy_2_1.h"
#include "data_obj/enemy_shot_0_0.h"
#include "data_obj/enemy_shot_0_1.h"
#include "data_obj/enemy_shot_1_0.h"
#include "data_obj/enemy_shot_1_1.h"
#include "data_obj/intro_0.h"
#include "data_obj/intro_1.h"
#include "data_obj/intro_2.h"
#include "data_obj/intro_3.h"
#include "data_obj/intro_4.h"
#include "data_obj/intro_5.h"
#include "data_obj/intro_6.h"
#include "data_obj/intro_7.h"
#include "data_obj/intro_8.h"
#include "data_obj/intro_9.h"
#include "data_obj/intro_10.h"
#include "data_obj/intro_11.h"
#include "data_obj/intro_12.h"
#include "data_obj/intro_13.h"
#include "data_obj/intro_14.h"
#include "data_obj/intro_15.h"
#include "data_obj/castle_top_left_0.h"
#include "data_obj/castle_top_left_1.h"
#include "data_obj/castle_top_left_2.h"
#include "data_obj/castle_top_left_3.h"
#include "data_obj/castle_bottom_left_0.h"
#include "data_obj/castle_bottom_left_1.h"
#include "data_obj/castle_bottom_left_2.h"
#include "data_obj/castle_bottom_left_3.h"
#include "data_obj/castle_top_right_0.h"
#include "data_obj/castle_top_right_1.h"
#include "data_obj/castle_top_right_2.h"
#include "data_obj/castle_top_right_3.h"
#include "data_obj/castle_bottom_right_0.h"
#include "data_obj/castle_bottom_right_1.h"
#include "data_obj/castle_bottom_right_2.h"
#include "data_obj/castle_bottom_right_3.h"
#include "data_obj/castle_solid_0.h"
#include "data_obj/castle_solid_1.h"
#include "data_obj/castle_solid_2.h"
#include "data_obj/castle_solid_3.h"
#include "data_obj/boss.h"
#include "data_obj/pause.h"
#include "data_obj/shot_wav.h"
#include "data_obj/kaboom.h"
#include "data_obj/ufo.h"
#include "data_obj/ufo_explode.h"
#include "data_obj/intro_ogg.h"
#include "data_obj/death_xm.h"
#include "data_obj/revisq_mod.h"
#include "data_obj/player_bmp.h"

extern SDL_Surface *screen;
extern SDL_Surface *icon;
extern SDL_Surface *title_screen;
extern SDL_Surface *shot_images[10];
extern SDL_Surface *enemy_images[3][10][2];
extern SDL_Surface *enemy_shot_images[2][2];
extern SDL_Surface *castle_bits[5][4];
#ifdef intro
extern SDL_Surface *intro_frames[16];
#endif

extern Mix_Chunk *sounds[NUM_WAVES];
extern Mix_Music *songs[NUM_SONGS];

extern object boss;
extern int points_lookup[11];
extern object player;
extern object explosions[MAX_EXPLOSIONS];   //player, boss
extern object enemy_shots[MAX_ENEMY_SHOTS];
extern int music_ok;
extern SFont_Font *font[7];
extern object shots[MAX_SHOTS];

extern Uint32 colours[NUM_COLOURS];

void __debug(const char *function, int lineno, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s():%-3d  ", function, lineno);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void setpixel(SDL_Surface * surface, int x, int y, Uint32 pixel)
{
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * 4;
    *(Uint32 *) p = pixel;
}

Uint32 getpixel(SDL_Surface * surface, int x, int y)
{
    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * 4;
    return *(Uint32 *) p;
}

SDL_Surface *tint(SDL_Surface * sprite, Uint32 to)
{
    int i, j;
    SDL_Surface *new;
    new = SDL_ConvertSurface(sprite, sprite->format, sprite->flags);

    SDL_LockSurface(new);
    for (i = 0; i < new->h; i++) {
        for (j = 0; j < new->w; j++) {
            if (getpixel(new, j, i) != colours[TRANSPARENT])
                setpixel(new, j, i, to);
        }
    }
    SDL_UnlockSurface(new);
    SDL_FreeSurface(sprite);
    return new;
}

SDL_Surface *transparent(SDL_Surface * surf)
{
    SDL_SetColorKey(surf, SDL_SRCCOLORKEY | SDL_RLEACCEL, colours[TRANSPARENT]);
    return surf;
}

void load_early_data(void)
{
    colours[BLACK] = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
    colours[BROWN] = SDL_MapRGB(screen->format, 0xA7, 0x40, 0x40);
    colours[RED] = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
    colours[ORANGE] = SDL_MapRGB(screen->format, 0xFF, 0x5e, 0x00);
    colours[YELLOW] = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0x00);
    colours[GREEN] = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
    colours[BLUE] = SDL_MapRGB(screen->format, 0x1e, 0x4b, 0xff);
    colours[VIOLET] = SDL_MapRGB(screen->format, 0xD0, 0x00, 0xD0);
    colours[GREY] = SDL_MapRGB(screen->format, 0x80, 0x80, 0x80);
    colours[WHITE] = SDL_MapRGB(screen->format, 0xFE, 0xFF, 0xFF);
    colours[TRANSPARENT] = SDL_MapRGB(screen->format, 0xFF, 0x00, 0xFF);
    title_screen = load_bmp(title_screen_png, title_screen_png_len);
    icon = transparent(load_bmp(icon_bmp, icon_bmp_len));

    font[0] = SFont_InitFont(load_bmp(font_bluish_png, font_bluish_png_len));
    font[1] = SFont_InitFont(load_bmp(font_blue_png, font_blue_png_len));
    font[2] = SFont_InitFont(load_bmp(font_yellow_png, font_yellow_png_len));
    font[3] = SFont_InitFont(load_bmp(font_red_png, font_red_png_len));
    font[4] = SFont_InitFont(load_bmp(font_white_png, font_white_png_len));
    font[5] = SFont_InitFont(load_bmp(font_green_png, font_green_png_len));
    font[6] = SFont_InitFont(load_bmp(font_grey_png, font_grey_png_len));
}

void load_data()
{
    int i, j;


    shot_images[0] = transparent(load_bmp(shot_bmp, shot_bmp_len));
    enemy_images[0][0][0] = transparent(load_bmp(enemy_0_0_bmp, enemy_0_0_bmp_len));
    enemy_images[0][0][1] = transparent(load_bmp(enemy_0_1_bmp, enemy_0_1_bmp_len));
    enemy_images[1][0][0] = transparent(load_bmp(enemy_1_0_bmp, enemy_1_0_bmp_len));
    enemy_images[1][0][1] = transparent(load_bmp(enemy_1_1_bmp, enemy_1_1_bmp_len));
    enemy_images[2][0][0] = transparent(load_bmp(enemy_2_0_bmp, enemy_2_0_bmp_len));
    enemy_images[2][0][1] = transparent(load_bmp(enemy_2_1_bmp, enemy_2_1_bmp_len));

    for (i = 1; i < 10; i++)
        shot_images[i] = tint(shot_images[0], colours[i]);

    shot_images[0] = tint(shot_images[0], colours[GREY]);

    // black doesn't need tint
    for (j = 0; j < 3; j++) {
        for (i = 1; i < 10; i++) {
            enemy_images[j][i][0] = tint(enemy_images[j][0][0], colours[i]);
            enemy_images[j][i][1] = tint(enemy_images[j][0][1], colours[i]);
        }
    }

    enemy_shot_images[0][0] = transparent(load_bmp(enemy_shot_0_0_bmp, enemy_shot_0_0_bmp_len));
    enemy_shot_images[0][1] = transparent(load_bmp(enemy_shot_0_1_bmp, enemy_shot_0_1_bmp_len));
    enemy_shot_images[1][0] = transparent(load_bmp(enemy_shot_1_0_bmp, enemy_shot_1_0_bmp_len));
    enemy_shot_images[1][1] = transparent(load_bmp(enemy_shot_1_1_bmp, enemy_shot_1_1_bmp_len));

    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
        enemy_shots[i].image = enemy_shot_images[0][0];

    for (i = 1; i < MAX_SHOTS; i++)
        shots[i].image = shot_images[shots[i].colour];

    explosions[0].image = transparent(load_bmp(blam_bmp, blam_bmp_len));

    for (i = 1; i < MAX_EXPLOSIONS; i++)
        explosions[i].image = explosions[0].image;

    intro_frames[0] = load_bmp(intro_0_png, intro_0_png_len);
    intro_frames[1] = load_bmp(intro_1_png, intro_1_png_len);
    intro_frames[2] = load_bmp(intro_2_png, intro_2_png_len);
    intro_frames[3] = load_bmp(intro_3_png, intro_3_png_len);
    intro_frames[4] = load_bmp(intro_4_png, intro_4_png_len);
    intro_frames[5] = load_bmp(intro_5_png, intro_5_png_len);
    intro_frames[6] = load_bmp(intro_6_png, intro_6_png_len);
    intro_frames[7] = load_bmp(intro_7_png, intro_7_png_len);
    intro_frames[8] = load_bmp(intro_8_png, intro_8_png_len);
    intro_frames[9] = load_bmp(intro_9_png, intro_9_png_len);
    intro_frames[10] = load_bmp(intro_10_png, intro_10_png_len);
    intro_frames[11] = load_bmp(intro_11_png, intro_11_png_len);
    intro_frames[12] = load_bmp(intro_12_png, intro_12_png_len);
    intro_frames[13] = load_bmp(intro_13_png, intro_13_png_len);
    intro_frames[14] = load_bmp(intro_14_png, intro_14_png_len);
    intro_frames[15] = load_bmp(intro_15_png, intro_15_png_len);

#define LOAD(i,text) \
  castle_bits[i][0] = transparent(load_bmp(castle_ ## text ## _0_bmp, castle_ ## text ## _0_bmp_len)); \
  castle_bits[i][1] = transparent(load_bmp(castle_ ## text ## _1_bmp, castle_ ## text ## _1_bmp_len)); \
  castle_bits[i][2] = transparent(load_bmp(castle_ ## text ## _2_bmp, castle_ ## text ## _2_bmp_len)); \
  castle_bits[i][3] = transparent(load_bmp(castle_ ## text ## _3_bmp, castle_ ## text ## _3_bmp_len));

    LOAD(0, top_left);
    LOAD(1, top_right);
    LOAD(2, solid);
    LOAD(3, bottom_left);
    LOAD(4, bottom_right);

    boss.image = transparent(load_bmp(boss_bmp, boss_bmp_len));

    points_lookup[0] = 1000;    // black
    points_lookup[1] = 750;     // brown
    points_lookup[2] = 500;     // red
    points_lookup[3] = 400;     // orange
    points_lookup[4] = 350;     // yellow
    points_lookup[5] = 300;     // green
    points_lookup[6] = 250;     // blue
    points_lookup[7] = 200;     // violet
    points_lookup[8] = 150;     // grey
    points_lookup[9] = 50;      // white
    points_lookup[10] = 5000;   // boss
    if (music_ok) {
        sounds[PAUSE_WAV] = load_sound(pause_ogg, pause_ogg_len);
        sounds[SHOT_WAV] = load_sound(shot_wav, shot_wav_len);
        sounds[EXPLODE_WAV] = load_sound(kaboom_ogg, kaboom_ogg_len);
        sounds[UFO_WAV] = load_sound(ufo_wav, ufo_wav_len);
        sounds[UFO_EXPLODE_WAV] = load_sound(ufo_explode_ogg, ufo_explode_ogg_len);
        songs[DEAD_SONG] = load_song(fred_ogg, fred_ogg_len);
        songs[INTRO_SONG] = load_song(intro_ogg, intro_ogg_len);
    }
    songs[LEVEL3_SONG] = load_song(death_xm, death_xm_len);
    songs[LEVEL1_SONG] = load_song(revisq_mod, revisq_mod_len);
    songs[LEVEL2_SONG] = load_song(death_xm, death_xm_len);
    player.image = transparent(load_bmp(player_bmp, player_bmp_len));
    if (music_ok) {
        songs[MENU_SONG] = load_song(menu_xm, menu_xm_len);
        if (!Mix_PlayingMusic()) {
            Mix_PlayMusic(songs[MENU_SONG], 0);
        }
    }
}


Mix_Chunk *load_sound(const void *data, unsigned int len)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, len);
    Mix_Chunk *sound = Mix_LoadWAV_RW(rw, 1);

    return sound;
}

Mix_Music *load_song(const void *data, unsigned int len)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, len);
    Mix_Music *song = Mix_LoadMUS_RW(rw);

    return song;
}

SDL_Surface *load_bmp(const void *data, unsigned int len)
{
    SDL_RWops *rw = SDL_RWFromConstMem(data, len);
    SDL_Surface *temp = IMG_Load_RW(rw, 1);

    SDL_Surface *image = SDL_DisplayFormat(temp);

    SDL_FreeSurface(temp);

    return image;
}
