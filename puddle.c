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
double DAMP = 0.95;
double K = 20;
double MAXDISP = 1;

static const char GREYSCALE[] = " .,:?)tuUO*%B@$#";

//----------------------------------------[Spring Structure]----------------------------------------
typedef double spring;
//typedef struct Spring{
//    float x; //displacement
//    float v; //velocity
//    float a; //acceleration
//} spring;

//---------------------------------------[Memory Management]----------------------------------------

spring** new_grid(size_t row, size_t col){
    spring** grid = (spring**) malloc(row * sizeof(spring*));
    for(size_t i = 0; i < row; i++){
        grid[i] = (spring*) calloc(col, sizeof(spring));
    }
    return grid;
}

void free_grid(spring** grid, size_t row){
    for(size_t i = 0; i < row; i++){
        free(grid[i]);
        grid[i] = NULL;
    }
    free(grid);
}

//-------------------------------------------[Simulation]-------------------------------------------
//https://web.archive.org/web/20160418004149/http://freespace.virgin.net/hugo.elias/graphics/x_water.htm

void simulate(spring*** buf1, spring*** buf2, size_t rows, size_t cols){
    for (size_t i = 1; i < rows - 1; i++){
        for (size_t j = 1; j < cols - 1; j++){
            (*buf2)[i][j]=(((*buf1)[i-1][j] +
                            (*buf1)[i+1][j] +
                            (*buf1)[i][j-1] +
                            (*buf1)[i][j+1]) / 2)
                            - (*buf2)[i][j];
            (*buf2)[i][j] *= DAMP;
        }
    }
}

//-------------------------------------------[Rendering]--------------------------------------------

void start_ncurses(){
    initscr();
#ifdef COLOR
    if (has_colors()){
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLUE);
        attron(COLOR_PAIR(1));
    }
#endif
    cbreak();
    curs_set(0); //Invisible cursor
    timeout(0); //make getch() nonblocking
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, HEIGHT, WIDTH);
    WIDTH /= 2; //Divide by 2 to make circular ripples instead of elliptical ones.
}

void printframe(spring** field, size_t row, size_t col){
    char ch;
    int len = strlen(GREYSCALE);
    for(int r = 1; r < row; r++){
        for(int c = 1; c < col; c++){
            move(r-1, (c*2)-1);
            int ch_val = MIN((int)(abs((float)len * field[r][c] / (float)MAXDISP)), len - 1);
            ch_val = MAX(0, ch_val);
            ch = GREYSCALE[ch_val];
            addch(ch);
            move(r-1, c*2);
            addch(ch);
        }
    }
    refresh();
}

void puddle(float intensity){
    int persecond = 1000000;
    int framerate = 20;
    int frameperiod = persecond / framerate;
    spring** field = new_grid(HEIGHT+2, WIDTH+2);
    spring** next = new_grid(HEIGHT+2, WIDTH+2);
    spring** temp;
    int c = 0;
    int x = rand() % WIDTH;
    int y = rand() % HEIGHT;
    int wait = rand() % (int)(framerate * 10 / intensity);
    int count = -1;
    while ((c = getch()) != 'q'){
        if (count == wait){
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
            wait = rand() % (int)(framerate * 10 / intensity);
            count = -1;
            field[y][x] += 4*(float)rand()/(float)(RAND_MAX/MAXDISP) - (MAXDISP/2);
        }
        printframe(field, HEIGHT, WIDTH);
        simulate(&field, &next, HEIGHT, WIDTH);

        temp = field;
        field = next;
        next = temp;

        usleep(frameperiod);
        count++;
    }
    free_grid(field, HEIGHT);
    free_grid(next, HEIGHT);
    field = NULL;
#ifdef COLOR
    if(has_colors()){
        attroff(COLOR_PAIR(1));
    }
#endif
    clear();
    refresh();
    endwin();
}

int main(int argc, char** argv){
    srand(time(NULL));
    int opt;
    opterr = 0;
    float intensity = 7;
    while ((opt = getopt(argc, argv, "d:i:k:")) != -1){
        switch (opt) {
            case 'd':
                DAMP = atof(optarg);
                break;
            case 'i':
                intensity = atof(optarg);
                break;
            case 'k':
                K = atof(optarg);
                break;
            default:
                return 1;
        }
    }

    start_ncurses();
    puddle(intensity);
    return 0;
}
