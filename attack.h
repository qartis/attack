#ifdef __WIN32__
#define edition 'w'
#else
#define edition 'l'
#endif

#define MAXVAL 100

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#ifdef debug
#define DBG(code) code
#else
#define DBG(code)
#endif
#define DEBUG(...) DBG(__debug(__FUNCTION__, __LINE__, __VA_ARGS__))

#define	FRAMES_PER_SEC	30
#define SCREEN_WIDTH    374
#define SCREEN_HEIGHT   480
#define MAX_ENEMIES     ENEMY_ROWS * ENEMY_COLUMNS

#define LIVES		3
#ifdef debug
#define MAX_SHOTS	3
#else
#define MAX_SHOTS 1
#endif
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

#define PLAYER_SPEED	4
#define SHOT_SPEED	12
#define ENEMY_SPEED	20
#define ENEMY_SHOT_SPEED 5
#define ENEMY_SHOT_FLICKER 3
#define ENEMY_SHOT_MIN_DELAY 30
#define ENEMY_SHOT_MAX_DELAY 60
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

#undef TRANSPARENT /* windows has TRANSPARENT defined somewhere */

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
    LEVEL2_SONG,
    LEVEL3_SONG,
    INTRO_SONG,
    NUM_SONGS
};

enum {
    LEFT,
    UP,
    DOWN,
    RIGHT
};

void setpixel(SDL_Surface * surface, int x, int y, Uint32 pixel);
Uint32 getpixel(SDL_Surface * surface, int x, int y);
void setfourpixels(object * castle, int x, int y);

SDL_Surface *tint(SDL_Surface * sprite, Uint32 to);

int can_shoot(int);
int dead_shot(void);
void player_hit(void);

void load_early_data(void);
void load_data(void);
void castle_hit(object * castle, int x, int y, int down);
int rand_between(int low, int high);
void wait(void);

int pixel_collision(object * sprite1, object * sprite2, int bottom);
int box_collision(object * sprite1, object * sprite2, int padding);

int find_enemy_who_can_shoot(void);
int need_reverse_enemies(void);

void draw_points(int disabled);
void show_title_screen(void);
void game(int, int);
void highscores();
void vidmode();
SDL_Surface *load_bmp(const void *data, unsigned int len);
Mix_Chunk *load_sound(const void *data, unsigned int len);
Mix_Music *load_song(const void *data, unsigned int len);
SDL_Surface *transparent(SDL_Surface *);
void mkcastle(castle *);
int castle_collision(object *, castle *);
void castle_block_hit(castle *, int);
void draw_castle(castle *);
void toggle_mute(void);
void __debug(const char *, int, const char *fmt, ...)
    __attribute__ ((format(printf, 3, 4)));
