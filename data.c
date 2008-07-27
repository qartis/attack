#include "attack.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#define SUFFIX "DATA"

#ifndef O_BINARY
#define O_BINARY 0
#endif

void __debug(const char *function, int lineno, const char *fmt, ...){
  const char *format = "%s():%-3d  ";
  va_list ap;

  fprintf(stderr, format, function, lineno);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr,"\n");
}

void setpixel(SDL_Surface * surface, int x, int y, Uint32 pixel){
  Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * 4;
  *(Uint32 *) p = pixel;
}

Uint32 getpixel(SDL_Surface * surface, int x, int y){
  Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * 4;
  return *(Uint32 *) p;
}

SDL_Surface *tint(SDL_Surface * sprite, Uint32 to){
  int i,j;
  SDL_Surface *new;
  new = SDL_ConvertSurface(sprite, sprite->format, sprite->flags);

  SDL_LockSurface(new);
  for (i = 0; i < new->h; i++) {
    for (j = 0; j < new->w; j++) {
      if (getpixel(new, j, i) != colours[TRANSPARENT]) setpixel(new, j, i, to);
    }
  }
  SDL_UnlockSurface(new);
  return new;
}

SDL_Surface* transparent(SDL_Surface* surf){
  SDL_SetColorKey(surf, SDL_SRCCOLORKEY | SDL_RLEACCEL,colours[TRANSPARENT]);
  return surf;
}

void load_early_data(void){
  colours[BLACK]       = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
  colours[BROWN]       = SDL_MapRGB(screen->format, 0xA7, 0x40, 0x40);
  colours[RED]         = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
  colours[ORANGE]      = SDL_MapRGB(screen->format, 0xFF, 0x5e, 0x00);
  colours[YELLOW]      = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0x00);
  colours[GREEN]       = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
  colours[BLUE]        = SDL_MapRGB(screen->format, 0x1e, 0x4b, 0xff);
  colours[VIOLET]      = SDL_MapRGB(screen->format, 0xD0, 0x00, 0xD0);
  colours[GREY]        = SDL_MapRGB(screen->format, 0x80, 0x80, 0x80);
  colours[WHITE]       = SDL_MapRGB(screen->format, 0xFE, 0xFF, 0xFF);
  colours[TRANSPARENT] = SDL_MapRGB(screen->format, 0xFF, 0x00, 0xFF);
  title_screen = load_bmp("title_screen.png");
  icon = transparent(load_bmp("icon.bmp"));

  font[0] = SFont_InitFont(load_bmp("font-bluish.png"));
  font[1] = SFont_InitFont(load_bmp("font-blue.png"));
  font[2] = SFont_InitFont(load_bmp("font-yellow.png"));
  font[3] = SFont_InitFont(load_bmp("font-red.png"));
  font[4] = SFont_InitFont(load_bmp("font-white.png"));
  font[5] = SFont_InitFont(load_bmp("font-green.png"));
  font[6] = SFont_InitFont(load_bmp("font-grey.png"));
}

void load_data(){
  int i,j;

  MUS(songs[MENU_SONG] = load_song("menu.xm.bz2");
      if (!Mix_PlayingMusic()){
        Mix_PlayMusic(songs[MENU_SONG], 0);
      });
  shot_images[0] = transparent(load_bmp("shot.bmp"));
  enemy_images[0][0][0] = transparent(load_bmp("enemy_0_0.bmp"));
  enemy_images[0][0][1] = transparent(load_bmp("enemy_0_1.bmp"));
  enemy_images[1][0][0] = transparent(load_bmp("enemy_1_0.bmp"));
  enemy_images[1][0][1] = transparent(load_bmp("enemy_1_1.bmp"));
  enemy_images[2][0][0] = transparent(load_bmp("enemy_2_0.bmp"));
  enemy_images[2][0][1] = transparent(load_bmp("enemy_2_1.bmp"));

  for (i = 1; i < 10; i++) shot_images[i] = tint(shot_images[0], colours[i]);
  shot_images[0] = tint(shot_images[0],SDL_MapRGB(screen->format,0x80,0x80,0x80));

  // black doesn't need tint
  for (j=0;j<3;j++){
    for (i = 1; i < 10; i++) {
      enemy_images[j][i][0] = tint(enemy_images[j][0][0], colours[i]);
      enemy_images[j][i][1] = tint(enemy_images[j][0][1], colours[i]);
    }
  }

  enemy_shot_images[0][0] = transparent(load_bmp("enemy_shot_0_0.bmp"));
  enemy_shot_images[0][1] = transparent(load_bmp("enemy_shot_0_1.bmp"));
  enemy_shot_images[1][0] = transparent(load_bmp("enemy_shot_1_0.bmp"));
  enemy_shot_images[1][1] = transparent(load_bmp("enemy_shot_1_1.bmp"));

  for (i = 0; i < MAX_ENEMY_SHOTS; i++)
    enemy_shots[i].image = enemy_shot_images[0][0];

  for (i = 1; i < MAX_SHOTS; i++)
    shots[i].image = shot_images[shots[i].colour];

  explosions[0].image = transparent(load_bmp("blam.bmp"));

  for (i = 1; i < MAX_EXPLOSIONS; i++)
    explosions[i].image = explosions[0].image;

  intro_frames[0] = load_bmp("intro_0.png");
  intro_frames[1] = load_bmp("intro_1.png");
  intro_frames[2] = load_bmp("intro_2.png");
  intro_frames[3] = load_bmp("intro_3.png");
  intro_frames[4] = load_bmp("intro_4.png");
  intro_frames[5] = load_bmp("intro_5.png");
  intro_frames[6] = load_bmp("intro_6.png");
  intro_frames[7] = load_bmp("intro_7.png");
  intro_frames[8] = load_bmp("intro_8.png");
  intro_frames[9] = load_bmp("intro_9.png");
  intro_frames[10] = load_bmp("intro_10.png");
  intro_frames[11] = load_bmp("intro_11.png");
  intro_frames[12] = load_bmp("intro_12.png");
  intro_frames[13] = load_bmp("intro_13.png");
  intro_frames[14] = load_bmp("intro_14.png");
  intro_frames[15] = load_bmp("intro_15.png");

#define LOAD(i,text) \
  castle_bits[i][0] = transparent(load_bmp("castle_" text "_0.bmp")); \
  castle_bits[i][1] = transparent(load_bmp("castle_" text "_1.bmp")); \
  castle_bits[i][2] = transparent(load_bmp("castle_" text "_2.bmp")); \
  castle_bits[i][3] = transparent(load_bmp("castle_" text "_3.bmp"));

  LOAD(0,"top_left");
  LOAD(1,"top_right");
  LOAD(2,"solid");
  LOAD(3,"bottom_left");
  LOAD(4,"bottom_right");

  boss.image = transparent(load_bmp("boss.bmp"));

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
  MUS(sounds[PAUSE_WAV] = load_sound("pause.ogg");
  sounds[SHOT_WAV] = load_sound("shot.wav");
  sounds[EXPLODE_WAV] = load_sound("kaboom.ogg");
  sounds[UFO_WAV] = load_sound("ufo.wav");
  sounds[UFO_EXPLODE_WAV] = load_sound("ufo_explode.ogg");
  songs[DEAD_SONG] = load_song("fred.ogg");
  songs[INTRO_SONG] = load_song("intro.ogg");)
  songs[LEVEL3_SONG] = load_song("death.xm.bz2");
  songs[LEVEL1_SONG] = load_song("revisq.mod.bz2");
  songs[LEVEL2_SONG] = load_song("death.xm.bz2");
  player.image = transparent(load_bmp("player.bmp"));
}

int open_resource(char *filename){
  int suffix_len = strlen(SUFFIX)+1;
  char buffer[suffix_len];
  int fd = open(filename, O_RDONLY|O_BINARY);
  if (fd < 0){
    DEBUG("Error opening myself for reading: %s",strerror(errno));
    return -1;
  }
  lseek(fd,-suffix_len,SEEK_END);
  buffer[suffix_len-1] = '\0';
  read(fd,buffer,suffix_len);
  if (0 != strncmp(buffer,SUFFIX,suffix_len)){
    close(fd);
    DEBUG("fatal error loading datafiles: invalid magic string %s",buffer);
    return -1;
  }
  return fd;
}

char *get_resource(char *resourcename, int *filesize) {
  uint32_t num_files;
  uint32_t temp;
  uint32_t i;
  uint32_t cur;
  uint32_t data_size = 0;
  char *filename;
  char *data = NULL;

  pthread_mutex_lock(&mutex);
  lseek(fd, -strlen(SUFFIX)-1, SEEK_END);
  lseek(fd,-(sizeof(uint32_t)),SEEK_CUR);
  read(fd,&num_files,sizeof(uint32_t));
  if (0 == num_files){
    DEBUG("fatal error: 0 resources");
    exit(EXIT_FAILURE);
  }
  lseek(fd,-(sizeof (uint32_t)),SEEK_CUR);
  i = num_files - 1;
  filename = NULL;
  do {
    lseek(fd,-(sizeof(uint32_t)),SEEK_CUR);
    read(fd,&cur,sizeof (uint32_t));
    lseek(fd,-(sizeof (uint32_t)),SEEK_CUR);
    if (i%2){
      data_size = cur;
    } else {
      off_t a = cur;
      lseek(fd,-a,SEEK_CUR);
      if (cur > 4096){
        DEBUG("error, cur=%u",cur);
        exit(EXIT_FAILURE);
      }
      filename = realloc(filename,cur+1);
      if (filename){
        memset(filename, 0, cur+1);
        read(fd,filename,cur);
        if (!strcmp(resourcename,filename)){
          *filesize = (int)data_size;
          lseek(fd,sizeof(uint32_t),SEEK_CUR);
          data = malloc(data_size);
          if (data){
            if ((temp=read(fd,data,data_size)) != data_size){
              DEBUG("could not read resource contents: %s, %d",resourcename,temp);
              exit(EXIT_FAILURE);
            }
          }
          break;
        }
      } else {
        DEBUG("no filename");
      }
    }
    off_t foo = cur;
    lseek(fd,-foo,SEEK_CUR);
  } while(i--);
  pthread_mutex_unlock(&mutex);
  if (!data){
    DEBUG("fatal error: garbage resource contents looking for %s (%d): %s",resourcename,i,filename);
    exit(EXIT_FAILURE);
  }
  free(filename);
  filename = NULL;

  return data;
}

Mix_Chunk *load_sound(char *filename){
  int filesize = 0;
  char *buffer = get_resource(filename, &filesize);

  SDL_RWops *rw = SDL_RWFromMem(buffer,filesize);
  Mix_Chunk *sound = Mix_LoadWAV_RW(rw, 1);

  free(buffer);
  buffer = NULL;

  return sound;
}

Mix_Music *load_song(char *filename){
  int filesize = 0;
  SDL_RWops *rw;
  char *buffer = get_resource(filename, &filesize);
  char *buffer2 = NULL;
  if (strcmp(strrchr(filename,'.'),".bz2")==0){
    unsigned int len = 2048*1024;
    buffer2 = malloc(len); /* bigger than the biggest resource file */
    int retval = BZ2_bzBuffToBuffDecompress(buffer2,&len,buffer,filesize,0,0);
    if (BZ_OK != retval){
      DEBUG("error decompressing bz2 data: %d",retval);
      exit(EXIT_FAILURE);
    }
    rw = SDL_RWFromMem(buffer2,len);
  } else {
    rw = SDL_RWFromMem(buffer,filesize);
  }
  Mix_Music *song = Mix_LoadMUS_RW(rw);
  if (!song) DEBUG("Mix_LoadMUS_RW error for file %s: %s",filename,Mix_GetError());
//  if (buffer2){free(buffer2);buffer2=NULL;} these cause weird issues: songs not playing, libvorbis freezing etc
//  if (buffer){free(buffer);buffer=NULL;}
  return song;
}


SDL_Surface *load_bmp(char *filename) {
  int filesize = 0;
  char *buffer = get_resource(filename, &filesize);

  SDL_RWops *rw = SDL_RWFromMem(buffer, filesize);
  SDL_Surface *temp = IMG_Load_RW(rw, 1);

  free(buffer);
  buffer = NULL;
  SDL_Surface *image;
  pthread_mutex_lock(&mutex);
  image = SDL_DisplayFormat(temp);
  pthread_mutex_unlock(&mutex);

  SDL_FreeSurface(temp);

  if (image == NULL){
    DEBUG("unable to load %s: %s",filename,SDL_GetError());
    exit(EXIT_FAILURE);
  }
  return image;
}
