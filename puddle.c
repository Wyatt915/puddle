#include <ncurses.h>
#include <signal.h>
#include <stdlib.h> //random, abs
#include <string.h>
#include <time.h>
#include <unistd.h> //usleep, getopt

#define SQRT2 1.41421356237

#define MIN(X, Y)  ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y)  ((X) > (Y) ? (X) : (Y))

size_t WIDTH;
size_t HEIGHT;
double MAXDISP = 1;

static const char GREYSCALE[] = " .,:?)tuUO*%B@$#";

//---------------------------------------[Memory Management]----------------------------------------

double** new_grid(size_t row, size_t col){
    double** grid = (double**) malloc(row * sizeof(double*));
    for(size_t i = 0; i < row; i++){
        grid[i] = (double*) calloc(col, sizeof(double));
    }
    return grid;
}

void free_grid(double** grid, size_t row){
    for(size_t i = 0; i < row; i++){
        free(grid[i]);
        grid[i] = NULL;
    }
    free(grid);
}

//-------------------------------------------[Simulation]-------------------------------------------
//https://web.archive.org/web/20160418004149/http://freespace.virgin.net/hugo.elias/graphics/x_water.htm

void simulate(double*** buf1, double*** buf2, size_t rows, size_t cols, double damp){
    //Remember that the buffers are larger than the screen area. there is a padding of 1 cell all
    //the way around, hence why we are able to access the memory location at i+1 and j+1 in all
    //cases.
    for (size_t i = 1; i < rows; i++){
        for (size_t j = 1; j < cols; j++){
            // Note that 11/32 + 5/32 = 1/2.
            (*buf2)[i][j]=(((*buf1)[i-1][j] +
                            (*buf1)[i+1][j] +
                            (*buf1)[i][j-1] +
                            (*buf1)[i][j+1]) * 11 / 32) + (( //We have added the edges, now do diagonals.
                            (*buf1)[i-1][j-1]/SQRT2 +
                            (*buf1)[i+1][j-1]/SQRT2 +
                            (*buf1)[i-1][j+1]/SQRT2 +
                            (*buf1)[i+1][j+1]/SQRT2) * 5 / 32)
                            - (*buf2)[i][j];
            (*buf2)[i][j] *= damp;
        }
    }
}

//---------------------------------------[NCURSES functions]----------------------------------------

int start_ncurses(){
    initscr();
#ifndef NOCOLOR
    if (has_colors()){
        start_color();
        if (COLORS < 256) { return -1; }
        //for whatever reason, white is #231, then #232 is dark grey, and the colors keep getting
        //lighter up to 255. so we need to start at #232 and place #231 at the end if we want white
        //to be included, or we can omit it and stick with 24 greyscale values.
        for(int i = 0; i < 24; i++){
            init_pair(i+1, i+232, i+232);
        }
    }
    else { return -1; }
#endif //NOCOLOR
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

int printframe(double** field, size_t row, size_t col){
    char ch = '#';
    int mag; //Magnitude of the ripple and therefore its color
    int len = sizeof(GREYSCALE);
    //Remember that extra padding around the buffers!
    for(int r = 1; r <= row; r++){
        for(int c = 1; c <= col; c++){
#ifdef NOCOLOR
            int ch_val = MIN((int)(abs((double)len * field[r][c] / (double)MAXDISP)), len - 1);
            ch_val = MAX(0, ch_val);
            ch = GREYSCALE[ch_val];
#else
            mag = MIN(abs((int)(24.0 * field[r][c] / (double)(MAXDISP))), 24);
            mag = MAX(mag, 1);
            attron(COLOR_PAIR(mag));
#endif //NOCOLOR
            //Subtracting 1 since the loop starts incrementing at 1.
            move(r-1, (c-1)*2);
            addch(ch); addch(ch);
            attroff(COLOR_PAIR(mag));
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
    double** temp;
    int c = 0;
    int x;
    int y;
    int wait = rand() % (int)(framerate * 10 / intensity);
    int count = -1;
    while ((c = getch()) != 'q'){
        if (count == wait){
            // If raindrops fall directly on the edge they get "stuck"
            x = 1 + rand() % (WIDTH - 2);
            y = 1 + rand() % (HEIGHT - 2);
            wait = rand() % (int)(framerate * 10 / intensity);
            count = -1;
            field[y][x] += 8*(double)rand()/(double)(RAND_MAX/MAXDISP) - (MAXDISP/2);
        }
        simulate(&field, &next, HEIGHT, WIDTH, damp);
        printframe(field, HEIGHT, WIDTH);

        //Swap the buffers
        temp = field;
        field = next;
        next = temp;

        usleep(frameperiod);
        count++;
    }
    free_grid(field, HEIGHT+2);
    free_grid(next, HEIGHT+2);
    field = NULL;
}

//----------------------------------------------[Main]----------------------------------------------

int main(int argc, char** argv){
    srand(time(NULL));
    int opt;
    opterr = 0;
    double intensity = 25;
    double damp = 0.95;

    const char helpmsg[] =
        "Usage: %s [flags]\n"\
        "\t-d\tSet the damping factor. A smaller damping factor\n"\
        "\t\tmeans ripples die out faster. Default is %g.\n"\
        "\t-i\tSet the rainfall intensity. A higher intensity means\n"\
        "\t\tmore raindrops per second. Default is %g.\n"\
        "\t-h\tShow this message and exit\n";

    const char colormsg[] =
        "%s requres 256-color support to run.\n"\
        "You can check if your terminal has 256 colors with\n"\
        "$ echo $TERM\n"\
        "(the output should read xterm-256color). When compiling,\n"\
        "make sure that you have ncurses ABI 6 or later and that\n"\
        "you link against ncurses with the flag -lncursesw if you\n"\
        "are not using the makefile for some reason.  If you are on\n"\
        "a system or terminal that does not have such capability,\n"\
        "you can compile with\n"\
        "$ make nocolor\n"\
        "or use the -DNOCOLOR flag with gcc.\n";

    while ((opt = getopt(argc, argv, "d:i:h")) != -1){
        switch (opt) {
            case 'd':
                damp = atof(optarg);
                break;
            case 'i':
                intensity = atof(optarg);
                break;
            case 'h':
            default:
                fprintf(stderr, helpmsg, argv[0], damp, intensity);
                return 0;
        }
    }

    if (start_ncurses() != 0){
        stop_ncurses();
        fprintf(stderr, colormsg, argv[0]);
        return 1;
    }

    puddle(intensity, damp);
    stop_ncurses();
    return 0;
}
