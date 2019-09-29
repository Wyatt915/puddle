/**************************************************************************************************
 *                                                                                                *
 *                                        .o8        .o8  oooo                                    *
 *                                       "888       "888  `888                                    *
 *           oo.ooooo.  oooo  oooo   .oooo888   .oooo888   888   .ooooo.       .ooooo.            *
 *            888' `88b `888  `888  d88' `888  d88' `888   888  d88' `88b     d88' `"Y8           *
 *            888   888  888   888  888   888  888   888   888  888ooo888     888                 *
 *            888   888  888   888  888   888  888   888   888  888    .o .o. 888   .o8           *
 *            888bod8P'  `V88V"V8P' `Y8bod88P" `Y8bod88P" o888o `Y8bod8P' Y8P `Y8bod8P'           *
 *            888                                                                                 *
 *           o888o                                                                                *
 *                                                                                                *
 *                         Starts a pleasant rainstorm in your terminal.                          *
 *                 On days like this, it's nice to just curl up with a good book.                 *
 *                                                                                                *
 *                                Copyright ⓒ 2019 Wyatt Sheffield                                *
 *                                                                                                *
 *                                                                                                *
 *         This program is free software: you can redistribute it and/or modify it under          *
 *         the terms of the GNU General Public License as published by the Free Software          *
 *           Foundation, either version 3 of the License, or (at your option) any later           *
 *                                            version.                                            *
 *                                                                                                *
 *          This program is distributed in the hope that it will be useful, but WITHOUT           *
 *         ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS          *
 *             FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more             *
 *                                            details.                                            *
 *                                                                                                *
 *            You should have received a copy of the GNU General Public License along             *
 *                with this program.  If not, see <https://www.gnu.org/licenses/>.                *
 *                                                                                                *
 *************************************************************************************************/

#include <ncurses.h>
#include <signal.h>
#include <stdlib.h> //random, abs
#include <string.h>
#include <time.h>
#include <unistd.h> //usleep, getopt

//---------------------------------------------[Macros]---------------------------------------------

#define MONO 0
#define BLUE 1

#define SQRT2 1.41421356237

#define MIN(X, Y)       ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)       ((X) > (Y) ? (X) : (Y))
#define CLAMP(X, A, B)  (MAX((A),MIN((X),(B))))

//--------------------------------------------[Globals]---------------------------------------------

// Width and height are initialized for debugging purposes.
uint16_t WIDTH = 100;
uint16_t HEIGHT = 100;

// Maximum displacement. This value is actually sometimes exceeded when raindrops fall, but we use
// this value to clamp the display scale
const double MAXDISP = 1;

// Used if colors are not supported in the terminal.
const char GREYSCALE[] = " .,:?)tuUO*%B@$#";
// xterm color codes
const int BLUES[] = { 16, 17, 18, 19, 20, 21, 26, 27, 32, 33, 39, 75, 81, 123, 159, 195, 231 };
//The number of colors available.
size_t SCALE;

//----------------------------------------[Signal Handling]-----------------------------------------

static volatile sig_atomic_t sig_caught = 0;

void sig_handler(int signum){
    sig_caught = signum;
}

//----------------------------[Memory Management and Utility Functions]-----------------------------

double** new_grid(size_t row, size_t col){
    double** grid = (double**) malloc(row * sizeof(double*));
    for(size_t i = 0; i < row; i++){
        grid[i] = calloc(col, sizeof(double));
    }
    return grid;
}

void copy_grid(double** dest, double** src, size_t row, size_t col){
    for(size_t i = 0; i < row; i++){
        for (size_t j = 0; j < col; j++){
            dest[i][j] = src[i][j];
        }
    }
}

void free_grid(double** grid, size_t row){
    for(size_t i = 0; i < row; i++){
        free(grid[i]);
        grid[i] = NULL;
    }
    free(grid);
}

//-------------------------------------------[Simulation]-------------------------------------------

// Modified from the following:
// https://web.archive.org/web/20160418004149/http://freespace.virgin.net/hugo.elias/graphics/x_water.htm
// with a slight tweak to make ripples more circular. All that entails is taking corner cells into
// consideration.

void simulate(double** buf1, double** buf2, size_t rows, size_t cols, double damp){
    // Remember that the buffers are larger than the screen area. there is a padding of 1 cell all
    // the way around, hence why we are able to access the memory location at i+1 and j+1 in all
    // cases. Similarly for i-1 and j-1.
    for (size_t i = 1; i <= rows; i++){
        for (size_t j = 1; j <= cols; j++){
            // Note that ⅓ + ⅙ = ½ and that ⅓ = 2 × ⅙
            buf2[i][j]=(
                   (buf1[i-1][j] +
                    buf1[i+1][j] +
                    buf1[i][j-1] +
                    buf1[i][j+1]) / 3) + (( //We have added the edges, now do corners.
                    buf1[i-1][j-1]/SQRT2 +
                    buf1[i+1][j-1]/SQRT2 +
                    buf1[i-1][j+1]/SQRT2 +
                    buf1[i+1][j+1]/SQRT2) / 6)
                    - buf2[i][j];
            buf2[i][j] *= damp;
        }
    }
}

//---------------------------------------[NCURSES functions]----------------------------------------

void set_palette(int p){
    switch (p){
        case BLUE:
            // Blue palette
            SCALE = sizeof(BLUES)/sizeof(int);
            for (int i = 0; i < SCALE; i++){
                init_pair(i+1, BLUES[i], BLUES[i]);
            }
            break;
        case MONO:
        default:
            // Monochrome palette
            //for whatever reason, white is #231, then #232 is dark grey, and the colors keep getting
            //lighter up to 255. so we need to start at #232 and place #231 at the end if we want white
            //to be included, or we can omit it and stick with 24 greyscale values.
            SCALE = 24;
            for(int i = 0; i < SCALE; i++){
                //Unfortunately, we have to start from 1 instead of 0.
                init_pair(i+1, i+232, i+232);
            }
        break;
    }
}

int start_ncurses(int p){
    initscr();
#ifndef NOCOLOR
    if (has_colors()){
        start_color();
        if (COLORS < 256) { return -1; }
        set_palette(p);
    }
    else { return -1; }
#else
    SCALE = (sizeof(GREYSCALE)/sizeof(char)) - 1;
#endif
    cbreak();
    curs_set(0); //Invisible cursor
    timeout(0); //make getch() nonblocking
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, HEIGHT, WIDTH);
    WIDTH /= 2; //Divide by 2 to make circular ripples instead of elliptical ones.
    return 0;
}

void stop_ncurses(){
    clear();
    refresh();
    endwin();
}

//----------------------------------------------[Draw]----------------------------------------------

int printframe(double** field, size_t row, size_t col){
    char ch = ' '; //The char to be printed
    int mag; //Magnitude of the ripple and therefore its color
    double disp;

    //Remember that extra padding around the buffers!
    for(int r = 1; r <= row; r++){
        for(int c = 1; c <= col; c++){
#ifdef NOCOLOR
            mag = abs((double)(SCALE-1) * field[r][c] / MAXDISP);
            mag = CLAMP(mag, 0, SCALE-1);
            ch = GREYSCALE[mag];
#else
            // We want the center value of our color palette to be 0, with the zeroth index being
            // at the minimum displacement and the last index at the maximum displacement.
            disp = 1 + ((double)SCALE/2) + ((double)SCALE * field[r][c] / (2 * MAXDISP));
            mag = CLAMP(disp, 1, SCALE);
            attron(COLOR_PAIR(mag));
#endif /* NOCOLOR */
            //Subtracting 1 since the loop starts incrementing at 1.
            move(r-1, (c-1)*2);
            addch(ch); addch(ch);
        }
    }
    refresh();
}

//------------------------------------------[Primary Loop]------------------------------------------

void puddle(double intensity, double damp){
    const int persecond = 1000000;
    int framerate = 30;
    intensity = MIN(intensity, framerate*10);
    int frameperiod = persecond / framerate;
    double** field = new_grid(HEIGHT+2, WIDTH+2);
    double** next = new_grid(HEIGHT+2, WIDTH+2);
    double** temp; // Used for swapping
    double** buf1; double** buf2; // Used for resizing
    int c = 0;
    int x;
    int y;
    int wait = rand() % (int)(framerate * 10 / intensity);
    int count = -1;
loop:
    while (getch() != 'q' && sig_caught == 0){
        if (count == wait){
            // If raindrops fall directly on the edge they get "stuck"
            x = 1 + rand() % (WIDTH - 2);
            y = 1 + rand() % (HEIGHT - 2);
            wait = rand() % (int)(framerate * 10 / intensity);
            count = -1;
            field[y][x] += 8*(double)rand()/(double)(RAND_MAX/MAXDISP) - 4;
        }
        simulate(field, next, HEIGHT, WIDTH, damp);
#ifndef DEBUG
        printframe(field, HEIGHT, WIDTH);
#endif
        //Swap the buffers
        temp = field;
        field = next;
        next = temp;

        usleep(frameperiod);
        count++;
    }

    if (sig_caught == SIGWINCH){
        endwin();
        refresh();
        size_t old_h = HEIGHT + 2; size_t old_w = WIDTH + 2;
        getmaxyx(stdscr, HEIGHT, WIDTH);
        //resizeterm(HEIGHT, WIDTH);
        WIDTH /= 2; //Divide by 2 to make circular ripples instead of elliptical ones.
        buf1 = new_grid(HEIGHT+2, WIDTH+2); buf2 = new_grid(HEIGHT+2, WIDTH+2);
        copy_grid(buf1, field, MIN(old_h, HEIGHT), MIN(old_w, WIDTH));
        copy_grid(buf2, next, MIN(old_h, HEIGHT), MIN(old_w, WIDTH));
        free_grid(field, old_h); free_grid(next, old_h);
        field = buf1; next = buf2;
        buf1 = NULL; buf2 = NULL;
        sig_caught = 0;
        goto loop;
    }
    free_grid(field, HEIGHT+2);
    free_grid(next, HEIGHT+2);
    field = NULL; next = NULL;
}

//----------------------------------------------[Main]----------------------------------------------

int main(int argc, char** argv){
    signal(SIGINT, sig_handler);
    signal(SIGWINCH, sig_handler);

    srand(time(NULL));
    double intensity = 75;
    double damp = 0.95;
    int palette = MONO;

    const char helpmsg[] =
        "Usage: %s [flags]\n"\
        "\t-d\tSet the damping factor. A smaller damping factor\n"\
        "\t\tmeans ripples die out faster. Default is %g.\n"\
        "\t-i\tSet the rainfall intensity. A higher intensity means\n"\
        "\t\tmore raindrops per second. Default is %g.\n"\
        "\t-p\tSelect the color palette to be used. 0 for monochrome\n"\
        "\t\tand 1 for blue. default is %d.\n"\
        "\t-h\tShow this message and exit\n";

    const char colormsg[] =
        "%s requres 256-color support to run.\n"\
        "You can check if your terminal has 256 colors with\n"\
        "$ echo $TERM\n"\
        "(the output should read xterm-256color). When compiling,\n"\
        "make sure that you have ncurses ABI 6 or later and that\n"\
        "you link against ncurses with the flag -lncursesw if you\n"\
        "are not using the makefile for some reason. If you are on\n"\
        "a system or terminal that does not have such capability,\n"\
        "you can compile with\n"\
        "$ make nocolor\n"\
        "or use the -DNOCOLOR flag with gcc.\n";

    int opt;
    opterr = 0;

    while ((opt = getopt(argc, argv, "d:i:hp:")) != -1){
        switch (opt) {
            case 'd':
                damp = atof(optarg);
                break;
            case 'i':
                intensity = atof(optarg);
                break;
            case 'p':
#ifdef NOCOLOR
                fprintf(stderr,
                        "%s has been compiled with NOCOLOR defined. The -p flag is unavailable.\n",
                        argv[0]);
                return 1;
#endif
                palette = atoi(optarg);
                break;
            case 'h':
            default:
                fprintf(stderr, helpmsg, argv[0], damp, intensity, palette);
                return 0;
        }
    }

    if (start_ncurses(palette) != 0){
        stop_ncurses();
        fprintf(stderr, colormsg, argv[0]);
        return 1;
    }

    puddle(intensity, damp);
    stop_ncurses();
    return 0;
}
