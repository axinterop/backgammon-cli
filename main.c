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

void draw_line_menu(WINDOW *menu_win, int y) {
    int maxx = getmaxx(menu_win);
    mvwaddch(menu_win, y, 0, ACS_LTEE);
    mvwhline(menu_win, y, 1, ACS_HLINE, maxx - 2);
    mvwaddch(menu_win, y, maxx - 1, ACS_RTEE);
}

void spacebar(WINDOW *win) {
    int c;
    if (win != NULL)
        wrefresh(win);
    refresh();
    while ((c = getch()) != ' ')
        ;
}

void init_POSITIONS() {
    coords_t positions[28] = {
        {50, 13}, {46, 13}, {42, 13}, {38, 13}, {34, 13}, {30, 13},

        {22, 13}, {18, 13}, {14, 13}, {10, 13}, {6, 13},  {2, 13},

        {2, 1},   {6, 1},   {10, 1},  {14, 1},  {18, 1},  {22, 1},

        {30, 1},  {34, 1},  {38, 1},  {42, 1},  {46, 1},  {50, 1},

        {54, 1},  // black home
        {26, 13}, // black bar

        {54, 13}, // white home
        {26, 1},  // white bar

    };
    FILE *fp;
    fp = fopen("POSITIONS", "wb");
    fwrite(positions, sizeof(positions), 1, fp);
    fclose(fp);
}

coords_t *load_POSITIONS() {
    coords_t *p = (coords_t *)malloc(sizeof(coords_t) * 28);
    FILE *fp;
    fp = fopen("POSITIONS", "rb");
    fread(p, sizeof(coords_t) * 28, 1, fp);
    fclose(fp);
    return p;
}

void prnt_mid(WINDOW *win, int starty, char *string, chtype color) {
    int length, x, y, width;
    float temp;

    if (win == NULL)
        win = stdscr;
    getyx(win, y, x);
    if (starty != 0)
        y = starty;
    width = getmaxx(win);

    length = strlen(string);
    temp = (width - length) / 2;
    x = (int)temp;
    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    wrefresh(win);
}

void save_game(board_t *board) {
    FILE *fp;
    fp = fopen("last_game", "wb");
    fwrite(board, sizeof(board_t), 1, fp);
    fclose(fp);
}

void load_game(board_t *board) {
    FILE *fp;
    fp = fopen("last_game", "rb");
    fread(board, sizeof(board_t), 1, fp);
    fclose(fp);
}

void init_players(player_t players[2]) {
    int player_num;
    for (player_num = 0; player_num <= 1; player_num++) {
        strcpy(players[player_num].nickname, "");
        for (int i = 0; i < PAWN_FIELDS; i++) {
            players[player_num].pawns[i] = 0;
        }
        players[player_num].pawns[BAR] = 0;
        players[player_num].pawns[HOME] = 0;
    }
}

void init_board(board_t *board) {
    init_players(board->players);

    int black_pos[] = {0, 11, 16, 18};
    int white_pos[] = {23, 12, 7, 5};
    int pawn_num[] = {2, 5, 3, 5};

    // Populate pawns
    for (int k = 0; k < 2; k++) {
        int current_plr = k;
        int *base_pos = black_pos;
        player_t *player_obj = &board->players[k];
        if (k == 1) {
            base_pos = white_pos;
        }
        for (int i = 0; i < 4; i++) {
            int pos = base_pos[i];
            player_obj->pawns[pos] = pawn_num[i];
        }
    }
    board->first = -1;
    board->total_m_n = 0;
}

int roll_d() { return rand() % 6 + 1; }

int ready_go_home(int calc_for, const player_t *players, int from_pos,
                  int target_pos) {
    const player_t *player = &players[calc_for];
    int pos, pawns_on_pos, sum = 0;

    int first_pos = 0;

    for (int i = 5; i >= 0; i--) {
        pos = abs(!calc_for * 23 - i); // If calc_for = WHITE -> positions <0;
                                       // 5>; else -> <18, 23> (homes)
        pawns_on_pos = player->pawns[pos];
        if (!first_pos && pawns_on_pos)
            first_pos = pos;
        sum += pawns_on_pos;
    }
    sum += player->pawns[HOME];
    if (from_pos == -1 && target_pos == -1 && sum == 15)
        return 1;
    if (sum != 15)
        return 0;
    if ((target_pos > HOME || target_pos < 0) && from_pos == first_pos)
        return 1;
    if (target_pos == HOME || target_pos == -1)
        return 1;
    return 0;
}

int calc_from_bar(const int d[4], int plr, const player_t *plrs, move_t *a_m) {
    int m_n = 0;
    const player_t *op_plr = &plrs[!plr];

    for (int i = 0; i < 4; i++) {
        if (d[i] <= 0)
            continue;
        for (int j = 0; j <= 5; j++) {
            int target =
                abs(23 * plr - j); // for white, if dice = 6, choose 18, not 23
            if (j == d[i] - 1 && op_plr->pawns[target] < 2) {
                a_m[m_n].from_pos = BAR;
                a_m[m_n].target_pos = target;
                a_m[m_n].remove_pawn = op_plr->pawns[target] == 1;
                a_m[m_n].dices_used = i;
                m_n++;
            }
        }
    }
    return m_n;
}

int calculate_sum(int cur_pos, const int d[4], int m_n, int plr,
                  const player_t *plrs, move_t *a_m) {
    int t;
    const player_t *op_player = &plrs[!plr];
    const int dir = plr == 1 ? -1 : 1;
    if (d[0] < 0 || d[1] < 0)
        return 0;
    t = cur_pos + (d[0] + d[1]) * dir;
    if (t >= 0 && t < NUM_OF_TRI && op_player->pawns[t] < 2) {
        a_m[m_n].from_pos = cur_pos;
        a_m[m_n].target_pos = t;
        a_m[m_n].dices_used = 12;
        a_m[m_n].remove_pawn = op_player->pawns[t] == 1;
        return 1;
    }
    return 0;
}

int move_legal(int f, int t, int plr, const player_t *plrs) {
    // 0 - not legal, 1 - legal no pawn, 2 - legal pawn, 3 - legal home
    if (t != f && t >= 0 && t < NUM_OF_TRI && plrs[!plr].pawns[t] < 2) {
        if (plrs[!plr].pawns[t] == 1)
            return 2;
        return 1;
    }
    if (ready_go_home(plr, plrs, f, t))
        return 3;
    return 0;
}

move_t construct_move(int f, int t, int d_u, int r_p) {
    move_t m;
    m.from_pos = f;
    m.target_pos = t;
    m.dices_used = d_u;
    m.remove_pawn = r_p;
    return m;
}

int calc_simple(const int d[4], int plr, const player_t *plrs, move_t *a_m) {
    const player_t *player = &plrs[plr];
    const player_t *op_player = &plrs[!plr];

    int m_n = 0;
    const int dir = plr == 1 ? -1 : 1;
    for (int cur_pos = 0; cur_pos < NUM_OF_TRI; cur_pos++) {
        if (player->pawns[cur_pos] == 0)
            continue;
        int target, legal;
        for (int i = 0; i < 4; i++) {
            if (d[i] < 0)
                continue;
            target = cur_pos + d[i] * dir;
            legal = move_legal(cur_pos, target, plr, plrs);
            if (legal) {
                target = (legal == 3) ? HOME : target;
                a_m[m_n] = construct_move(cur_pos, target, i,
                                          op_player->pawns[target] == 1);
                m_n++;
            }
        }
        if (calculate_sum(cur_pos, d, m_n, plr, plrs, a_m))
            m_n += 1;
    }
    return m_n;
}

void show_game_options(int t, int first_plr) {
    int start_y = (LINES - BOARD_HEIGHT) / 4 + 4;
    int start_x = (COLS + BOARD_WIDTH) / 2 + 4;
    char *ops[4] = {"(M)ove (Enter)", "(S)ave game", "", "(E)xit"};
    if (t) {
        for (int i = 0; i < 4; i++) {
            attron(COLOR_PAIR(first_plr + 1));
            mvprintw(start_y + i, start_x, "%s", ops[i]);
            attroff(COLOR_PAIR(first_plr + 1));
        }
    } else
        for (int i = 0; i < 4; i++)
            mvhline(start_y + i, start_x, ' ', 20);
    refresh();
};

int get_move(int p, int t, move_t *a_m, int m_n, move_t *move) {
    for (int i = 0; i < m_n; i++) {
        if (p == a_m[i].from_pos && t == a_m[i].target_pos) {
            *move = a_m[i];
            return 1;
        }
    }
    return 0;
}

int get_options_from(int *f_ops, const move_t *a_m, int m_n) {
    int move, k;
    int prev = -1, r_m = 0;
    for (move = 0, k = 0; move < m_n; move++) {
        int current_from = a_m[move].from_pos;
        if (current_from != prev) {
            f_ops[k++] = current_from;
            prev = current_from;
        }
    }
    return k;
}

int get_options_target(int f_pos, int *t_ops, move_t *a_m, int m_n, int plr,
                       player_t *plrs) {
    int move, k, r_m = 0;
    for (move = 0, k = 0; move < m_n; move++) {
        if (a_m[move].from_pos == f_pos)
            t_ops[k++] = a_m[move].target_pos;
    }
    return k;
}

void choose_option(int *op, int *confirm, int op_num, int dir) {
    int c;
    c = getch();
    if (c == KEY_RIGHT)
        *op = (op_num + (*op - 1 * dir)) % op_num;
    else if (c == KEY_LEFT)
        *op = (op_num + (*op + 1 * dir)) % op_num;
    else if (c == 10)
        *confirm = 1;
}

int ask_for_pawn(WINDOW *local_win, int dir, move_t *a_m, int m_n, int calc_for,
                 player_t *players) {
    int f_ops[MAX_MOVES];
    int f_num = get_options_from(f_ops, a_m, m_n);
    int p_op = 0, confirm = 0;
    while (!confirm) {
        draw_pawn_options(local_win, p_op, calc_for, players, f_ops, f_num);
        choose_option(&p_op, &confirm, f_num, dir);
    }
    return f_ops[p_op];
}

int ask_for_tri(WINDOW *local_win, int p_pos, int dir, move_t *a_m, int m_n,
                int calc_for, player_t *players) {
    int t_ops[MAX_MOVES];
    int t_num = get_options_target(p_pos, t_ops, a_m, m_n, calc_for, players);
    int tri_op = 0, confirm = 0;
    while (!confirm) {
        draw_tri_options(local_win, tri_op, calc_for, t_ops, t_num);
        choose_option(&tri_op, &confirm, t_num, dir);
    }
    return t_ops[tri_op];
}

int ask_for_move(WINDOW *game_win, move_t *move, int plr, player_t *plrs,
                 move_t *a_m, int m_n) {
    wrefresh(game_win);
    int dir = plr ? -1 : 1;
    int pawn_pos, tri_pos;
    pawn_pos = ask_for_pawn(game_win, dir, a_m, m_n, plr, plrs);
    tri_pos = ask_for_tri(game_win, pawn_pos, dir, a_m, m_n, plr, plrs);
    if (get_move(pawn_pos, tri_pos, a_m, m_n, move))
        return 1;
    return 0;
}

void execute_move(const move_t move, int calc_for, player_t *players) {
    player_t *player = &players[calc_for];
    player_t *op_player = &players[!calc_for];
    player->pawns[move.from_pos] -= 1;
    if (move.remove_pawn) {
        op_player->pawns[move.target_pos] = 0;
        op_player->pawns[BAR] += 1;
    }
    player->pawns[move.target_pos] += 1;
}

void construct_dice_m(int m[9], int d) {
    if (d % 2 == 1)
        m[4] = 1;
    if (d / 2 != 0)
        m[0] = m[8] = 1;
    if (d / 4 != 0)
        m[2] = m[6] = 1;
    if (d == 6)
        m[3] = m[5] = 1;
}

void draw_dice_box(WINDOW *win, int starty, int startx) {
    mvwhline(win, starty, startx, ACS_HLINE, 3 * 3);
    mvwhline(win, starty + 4, startx, ACS_HLINE, 3 * 3);
    mvwvline(win, starty + 1, startx, ACS_VLINE, 3);
    mvwvline(win, starty + 1, startx + 3 * 3 - 1, ACS_VLINE, 3);
    mvwaddch(win, starty, startx, ACS_ULCORNER);
    mvwaddch(win, starty, startx + 3 * 3 - 1, ACS_URCORNER);
    mvwaddch(win, starty + 4, startx, ACS_LLCORNER);
    mvwaddch(win, starty + 4, startx + 3 * 3 - 1, ACS_LRCORNER);
}

void draw_dice(const int d, WINDOW *win, int starty, int startx) {
    int m[9] = {0};
    if (d > 0) {
        construct_dice_m(m, d);
        draw_dice_box(win, starty, startx);
    }
    if (win == NULL)
        win = stdscr;
    starty++;
    startx += 2;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) {
            wmove(win, starty + r, startx + c * 2);
            if (*(m + 3 * r + c))
                wprintw(win, "* ");
            else
                wprintw(win, "  ");
        }
    wrefresh(win);
}

void clear_dice_board(WINDOW *win) {
    wclear(win);
    box(win, 0, 0);
    wrefresh(win);
}

void draw_dices(const int d[], WINDOW *dice_win, int starty, int startx,
                int between) {
    if (between < 10)
        between = 10;
    if (starty <= 0)
        starty = 1;
    if (startx <= 0)
        startx = 3;
    if (dice_win == NULL)
        dice_win = stdscr;
    clear_dice_board(dice_win);
    for (int i = 0; i < 4; i++)
        draw_dice(d[i], dice_win, starty, startx + between * i);
    refresh();
}

void draw_empty_dice_board(WINDOW *dice_win) {
    wclear(dice_win);
    box(dice_win, 0, 0);
    wrefresh(dice_win);
}

void draw_determine(WINDOW *g_w, int p1, int p2) {
    wclear(g_w);
    box(g_w, 0, 0);
    prnt_mid(g_w, 2, "Diciding who moves first...", 0);
    mvwprintw(g_w, 4, 15, "Player 1:");
    draw_dice(p1, g_w, 5, 15);
    mvwprintw(g_w, 4, BOARD_WIDTH / 2 + 5, "Player 2:");
    draw_dice(p2, g_w, 5, BOARD_WIDTH / 2 + 5);
    spacebar(g_w);
}

int determine_first(WINDOW *game_win) {
    int p1, p2, f = -1;
    while (f == -1) {
        p1 = roll_d();
        p2 = roll_d();
        draw_determine(game_win, p1, p2);
        if (p1 > p2) {
            f = PLAYER_BLACK;
            prnt_mid(game_win, 11, "Player 1 moves first!", COLOR_PAIR(f + 1));
        } else if (p1 < p2) {
            f = PLAYER_WHITE;
            prnt_mid(game_win, 11, "Player 2 moves first!", COLOR_PAIR(f + 1));
        } else if (p1 == p2)
            prnt_mid(game_win, 11, "Draw! Rolling again...", 0);

        spacebar(game_win);
    }
    return f;
}

int dicing(int d[4]) {
    int d1 = roll_d();
    int d2 = roll_d();
    if (d1 == d2)
        for (int i = 0; i < 4; i++)
            d[i] = d1;
    else {
        d[0] = d1;
        d[1] = d2;
        d[2] = d[3] = 0;
    }
    return d1 == d2;
}

void _p_s_nick(WINDOW *win, player_t *player) {
    char nick[MAX_NICK_LEN];
    echo();
    wattron(win, A_BOLD);
    wgetstr(win, nick);
    wattroff(win, A_BOLD);
    noecho();
    strcpy(player->nickname, nick);
}

void prompt_nicks(WINDOW *game_win, player_t *players) {
    for (int plr = 0; plr <= 1; plr++)
        if (strcmp(players[plr].nickname, "") != 0)
            return;

    wclear(game_win);

    curs_set(1);
    box(game_win, 0, 0);
    prnt_mid(game_win, 2, "Player 1 name?", 0);
    wmove(game_win, 2 + 3, (getmaxx(game_win) - 10) / 2);
    _p_s_nick(game_win, &players[0]);

    prnt_mid(game_win, 8, "Player 2 name?", 0);
    wmove(game_win, 8 + 3, (getmaxx(game_win) - 10) / 2);
    _p_s_nick(game_win, &players[1]);
    curs_set(0);

    wclear(game_win);
}

void draw_board(WINDOW *local_win, const board_t *board) {
    draw_empty_board(local_win);
    draw_full_board(local_win, board);
}

void draw_boards(WINDOW *game_win, WINDOW *dice_win, board_t *board) {
    draw_board(game_win, board);
    draw_empty_dice_board(dice_win);
}

int get_game_option() {
    int c;
    while (1 == 1) {
        c = getch();
        if (c == 'm' || c == 10) {
            return GAME_MOVE;
        } else if (c == 's') {
            return GAME_SAVE;
        } else if (c == 'e') {
            return GAME_EXIT;
        }
    }
}

void show_no_move(WINDOW *game_win) {
    int y, x;
    getmaxyx(game_win, y, x);
    char msg[] = "No legal moves (spacebar...)";
    int msg_len = strlen(msg);
    int printy = y / 2;
    int printx = (x - msg_len) / 2;
    mvwprintw(game_win, printy, printx, msg);
    spacebar(game_win);
    wrefresh(game_win);
}

void show_nicks(player_t *plrs) {
    int y1, y2, x1, x2;
    x1 = (COLS - BOARD_WIDTH) / 2 + BOARD_WIDTH -
         strlen(plrs[PLAYER_BLACK].nickname);
    ;
    y1 = (LINES - BOARD_HEIGHT) / 4 + BOARD_HEIGHT;
    x2 = (COLS - BOARD_WIDTH) / 2 + BOARD_WIDTH -
         strlen(plrs[PLAYER_WHITE].nickname);
    ;
    y2 = (LINES - BOARD_HEIGHT) / 4 - 1;

    attron(COLOR_PAIR(PLAYER_BLACK + 1));
    mvprintw(y1, x1, "%s", plrs[PLAYER_BLACK].nickname);
    attroff(COLOR_PAIR(PLAYER_BLACK + 1));

    attron(COLOR_PAIR(PLAYER_WHITE + 1));
    mvprintw(y2, x2, "%s", plrs[PLAYER_WHITE].nickname);
    attroff(COLOR_PAIR(PLAYER_WHITE + 1));

    refresh();
}

int reduce_dices(const move_t *m, int d[4]) {
    if (m->dices_used == 12) {
        d[0] *= -1;
        d[1] *= -1;
        return 2;
    } else {
        d[m->dices_used] *= -1;
        return 1;
    }
}

int force_bicie(move_t *f_b, move_t *a_m, int m_n, int plr) {
    int start = 0, end = m_n - 1, step = 1, k = 0;
    if (plr == 0) {
        start = m_n - 1;
        end = 0;
        step = -1;
    }
    for (int m = start; m != end; m += step)
        if (a_m[m].remove_pawn == 1) {
            f_b[k++] = a_m[m];
            break;
        }
    return k;
}

move_t *check_force_bicie(move_t *a_m, int *m_n, int plr) {
    move_t *f_b;
    f_b = (move_t *)malloc(sizeof(move_t) * MAX_MOVES);
    int fb;
    fb = force_bicie(f_b, a_m, *m_n, plr);

    if (fb) {
        *m_n = fb;
        return f_b;
    } else
        return a_m;
}

int calc_moves(const int d[4], int plr, const player_t *plrs, move_t *a_m) {
    int m_n = 0;
    if (plrs[plr].pawns[BAR])
        m_n = calc_from_bar(d, plr, plrs, a_m);
    else
        m_n = calc_simple(d, plr, plrs, a_m);
    return m_n;
}

void make_moves(WINDOW *g_w, WINDOW *d_w, int plr, board_t *b) {
    int d[4];
    int m_n, moves = 2 + 2 * dicing(d);
    move_t a_m[MAX_MOVES] = {};
    move_t c_move, *ch_a_m;
    player_t *plrs = b->players;

    draw_dices(d, d_w, -1, -1, -1);
    do {
        m_n = calc_moves(d, plr, plrs, a_m);
        if (!m_n) {
            show_no_move(g_w);
            return;
        }
        ch_a_m = a_m; // check_force_bicie(a_m, &m_n, plr);
        ask_for_move(g_w, &c_move, plr, plrs, ch_a_m, m_n); // can return value
        execute_move(c_move, plr, plrs);
        moves -= reduce_dices(&c_move, d);

        draw_boards(g_w, d_w, b);
        draw_dices(d, d_w, -1, -1, -1);

    } while (moves);
}

int check_win(WINDOW *g_w, WINDOW *d_w, board_t *b) {
    char nick[MAX_NICK_LEN];
    int w = 0;
    for (int p = 0; p <= 1; p++) {
        if (b->players[p].pawns[HOME] == 15) {
            strcpy(nick, b->players[p].nickname);
            w++;
        }
    }
    if (w) {
        char *msg = " won!";
        char *r = (char *)malloc(sizeof(char) * (MAX_NICK_LEN + 10));
        r = strcat(nick, msg);
        clear();
        refresh();
        wclear(g_w);
        wclear(d_w);
        wrefresh(d_w);
        box(g_w, 0, 0);
        prnt_mid(g_w, BOARD_HEIGHT / 2, r, COLOR_PAIR(10));
        spacebar(g_w);
    }
    return w;
}

void play_game(WINDOW *g_w, WINDOW *d_w, board_t *b) {
    prompt_nicks(g_w, b->players);
    if (b->first == -1) {
        b->first = determine_first(g_w);
    }
    show_nicks(b->players);
    int first_plr = (b->total_m_n + b->first) % 2;

    int RUN_GAME = 1;
    while (RUN_GAME) {
        if (check_win(g_w, d_w, b))
            return;
        draw_boards(g_w, d_w, b);
        int o, p = 2;
        while (p) {
            show_game_options(1, first_plr);
            o = get_game_option();
            switch (o) {
            case GAME_MOVE:
                show_game_options(0, -1);
                make_moves(g_w, d_w, first_plr, b);
                b->total_m_n++;
                break;
            case GAME_SAVE:
                save_game(b);
                continue;
            case GAME_EXIT:
                clear();
                refresh();
                return;
            }
            p--;
            first_plr = !first_plr;
        }
    }
}

void draw_menu(WINDOW *menu_win) {
    int starty = 4, startx = 3;
    char *choices[] = {
        "(1) Two players",
        "(-) Against computer",
        "(3) Load game",
        "(-) Hall of Fame",
        "",
        "(E)xit",
    };
    int choice_num = ARRAY_SIZE(choices);
    box(menu_win, 0, 0);
    prnt_mid(menu_win, 1, "PLAY BACKGAMMON", COLOR_PAIR(10));
    prnt_mid(menu_win, getmaxy(menu_win) - 2, "Made by: PG student", 0);
    draw_line_menu(menu_win, 2);
    draw_line_menu(menu_win, getmaxy(menu_win) - 3);
    wmove(menu_win, 3, 1);
    for (int i = 0; i < choice_num; i++) {
        mvwprintw(menu_win, i + starty, startx, "%s", choices[i]);
    }
    wrefresh(menu_win);
    refresh();
}

void init_system() {
    init_POSITIONS();
    start_color();
    init_pair(PLAYER_BLACK + 1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(PLAYER_WHITE + 1, COLOR_BLUE, COLOR_BLACK);
    init_pair(10, COLOR_CYAN, COLOR_BLACK);
    init_pair(PLAYER_BLACK + 3, COLOR_RED, COLOR_BLACK);
    init_pair(PLAYER_WHITE + 3, COLOR_WHITE, COLOR_BLACK);
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
}

int show_menu(WINDOW *m_m, WINDOW *g_w, WINDOW *d_w, board_t *b) {
    int c;
    while (c != 'e') {
        refresh();
        draw_menu(m_m);
        c = getch();
        if (c == '1') {
            init_board(b);
            play_game(g_w, d_w, b);
        }
        if (c == '3') {
            load_game(b);
            play_game(g_w, d_w, b);
        }
    }
    return 0;
}

int main() {
    srand(time(NULL));
    board_t board;

    WINDOW *g_w, *m_w, *d_w;
    int sx, sy, w, h;

    initscr();
    init_system();

    h = BOARD_HEIGHT;
    w = BOARD_WIDTH;
    sy = (LINES - h) / 4; /* Calculating for a center placement */
    sx = (COLS - w) / 2;  /*   of the window		*/

    g_w = create_newwin(h, w, sy, sx);
    m_w = create_newwin(h, w, sy, sx);
    d_w = create_newwin(DICE_HEIGHT + 2, w, sy + h + 2, sx);
    refresh();

    if (!show_menu(m_w, g_w, d_w, &board)) {
        destroy_win(m_w);
        destroy_win(g_w);
        destroy_win(d_w);
        endwin();
    }
    return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    return local_win;
}

void destroy_win(WINDOW *local_win) {
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}

void draw_board_lines(WINDOW *g_w) {
    coords_t add_bor[4] = {
        {25, 0},
        {28, 0},
        {53, 0},

    };
    int height = getmaxy(g_w);
    ;
    for (int i = 0; i < 3; i++) {
        int sym;
        for (int j = 0; j < height; j++) {
            if (j == 0)
                sym = ACS_TTEE;
            else if (j == height - 1)
                sym = ACS_BTEE;
            else
                sym = ACS_VLINE;
            mvwaddch(g_w, add_bor[i].y + j, add_bor[i].x, sym);
        }
    }
}

void draw_empty_board(WINDOW *g_w) {
    coords_t *pos = load_POSITIONS();
    wclear(g_w);
    box(g_w, 0, 0);
    for (int i = 0; i < NUM_OF_TRI; i++) {
        int direction = pos[i].y == 1 ? 1 : -1;
        char *const sym = i % 2 == 0 ? "--" : "::";
        for (int j = 0; j < MAX_PAWNS_ON_TRI; j++) {
            mvwprintw(g_w, pos[i].y + j * direction, pos[i].x, sym);
        }
    }
    for (int i = NUM_OF_TRI; i <= 27; i++) {
        if (i % 2 == 0)
            mvwprintw(g_w, pos[i].y, pos[i].x, " 0");
        else
            mvwprintw(g_w, pos[i].y, pos[i].x, "  ");
    };
    draw_board_lines(g_w);
}

void draw_pawns(WINDOW *g_w, int dir, int p_n, int p_diff, int plr_n, int pos_y,
                int pos_x) {
    char *const sym = plr_n ? "oo" : "TT";
    for (int j = 0; j < p_n; j++) {
        mvwprintw(g_w, pos_y + j * dir, pos_x, sym);
    }
    if (p_diff > 0) {
        int y, x;
        p_diff++;
        const char sym2[2] = {p_diff < 10 ? '+' : '1',
                              (char)((p_diff % 10) + 48)};
        getyx(g_w, y, x);
        mvwprintw(g_w, y, x - 2, "%s",
                  sym2); // x - 2 because previous mvwprintw moved the cursor
        wrefresh(g_w);
    }
}

void draw_full_board(WINDOW *g_w, const board_t *b) {
    int p_n, pos_offs = 0, plr_n = 1;
    int i;
    int dir, p_diff, pos_y, pos_x;
    coords_t *pos = load_POSITIONS();
    while (plr_n >= 0) { // First, draw everything for black plr
        player_t plr = b->players[plr_n];
        for (i = 0; i < PAWN_FIELDS; i++) {
            pos_offs = (i >= 24) * plr_n * 2;
            pos_y = pos[i + pos_offs].y;
            pos_x = pos[i].x;
            p_n = plr.pawns[i];
            if (!p_n)
                continue;
            dir = pos_y == 1 ? 1 : -1;
            p_diff = p_n - MAX_PAWNS_ON_TRI;
            if (p_diff > 0)
                p_n = MAX_PAWNS_ON_TRI;
            draw_pawns(g_w, dir, p_n, p_diff, plr_n, pos_y, pos_x);
        }
        plr_n--;
    }
    wrefresh(g_w);
}

void draw_pawn_options(WINDOW *g_w, int p_op, int plr, player_t *plrs, int *ops,
                       int num) {
    coords_t *POS = load_POSITIONS();
    int from_pos, corr_pos, dir, offset, new_y, attr;
    for (int from_op = 0; from_op < num; from_op++) {
        from_pos = ops[from_op];

        corr_pos = from_pos + (plr * 2) * (from_pos == 25);
        coords_t c_pos = POS[corr_pos];

        dir = c_pos.y == 1 ? 1 : -1;

        offset = plrs[plr].pawns[from_pos] - 1;
        if (offset > 5)
            offset = 5;
        new_y = c_pos.y + offset * dir;

        // if cursor is on user's choice -> attr = A_REVERSE
        attr = (from_pos == ops[p_op]) ? A_REVERSE : A_NORMAL;
        mvwchgat(g_w, new_y, c_pos.x, 2, attr, plr + 1, NULL);
        wmove(g_w, 0, 0);
        wrefresh(g_w);
        refresh();
    }
}

void draw_tri_options(WINDOW *local_win, int tri_op, int calc_for, int *options,
                      int num) {
    coords_t *positions = load_POSITIONS();
    int target_pos;
    for (int target_op = 0; target_op < num; target_op++) {
        target_pos = options[target_op];

        int corr_pos =
            target_pos +
            (calc_for * 2) * (target_pos == 24); // calculation for white bar
        coords_t c_pos = positions[corr_pos];

        int direction = c_pos.y == 1 ? 1 : -1;

        int attr = (target_pos == options[tri_op]) ? A_BOLD : A_NORMAL;
        for (int j = 0; j < MAX_PAWNS_ON_TRI; j++) {
            mvwchgat(local_win, c_pos.y + j * direction, c_pos.x, 2, attr,
                     calc_for + 1, NULL);
        }
        wmove(local_win, 0, 0);
        wrefresh(local_win);
        refresh();
    }
}
