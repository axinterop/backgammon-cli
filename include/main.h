#include <menu.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Make corrections if necessary
#define MAX_MOVES 32
#define MAX_PAWNS_ON_TRI 6 // MAX 6

// If you change something below it won't be backgammon anymore...
#define NUM_OF_TRI 24
#define BAR_HOME 2
#define PAWN_FIELDS (BAR_HOME + NUM_OF_TRI)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define BOARD_HEIGHT 15
#define BOARD_WIDTH 57
#define DICE_HEIGHT 5
#define DICE_WIDTH 9
#define QUIT_OPTION 100
#define GAME_MOVE 50
#define GAME_SAVE 51
#define GAME_EXIT 52

#define MAX_NICK_LEN 32

enum { PLAYER_BLACK, PLAYER_WHITE };

typedef struct {
    int x;
    int y;
} coords_t;

typedef struct {
    int from_pos;
    int target_pos;
    int remove_pawn;
    int dices_used; // 0 - 3
} move_t;

typedef struct {
    int pawns[PAWN_FIELDS];
    enum {
        HOME = PAWN_FIELDS - BAR_HOME,
        BAR,
    } home_bar;
    int dices[4];
    char nickname[MAX_NICK_LEN];
} player_t;

typedef struct {
    player_t players[2];
    int first;
    int total_m_n;
} board_t;

WINDOW *create_newwin(int height, int width, int starty, int startx);

void destroy_win(WINDOW *local_win);

void draw_empty_board(WINDOW *);

void draw_full_board(WINDOW *, const board_t *);

void draw_pawn_options(WINDOW *g_w, int p_op, int plr, player_t *plrs, int *ops,
                       int num);

void draw_tri_options(WINDOW *local_win, int tri_op, int calc_for, int *options,
                      int num);


