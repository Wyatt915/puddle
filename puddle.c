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
double DAMP = 0.99;
double K = 10;
double MAXDISP = 100;

static const char GREYSCALE[] = " .,:?)tuUO*%B@$#";

//----------------------------------------[Spring Structure]----------------------------------------

typedef struct Spring{
    float x; //displacement
    float v; //velocity
    float a; //acceleration
} spring;

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

//Propagate foces to neighboring springs and simulate
void simulate(spring*** cur, size_t row, size_t col, double frameperiod){
    spring** temp = new_grid(row, col);
    size_t resolution = 1000;
    double dt = 0.001;
    //double dt = frameperiod / (double) resolution;
    for(size_t i = 0; i < resolution; i++){
        for(int r = 0; r < row; r++){
            for(int c = 0; c < col; c++){
                temp[r][c].x = (*cur)[r][c].x + (dt * (*cur)[r][c].v) + (dt * dt * (*cur)[r][c].a)/2;
                temp[r][c].v = (*cur)[r][c].v + (dt * (*cur)[r][c].a);
                //add up all forces (equivalent to acceleration with m=1)
                if (r > 0)      temp[r][c].a -= (*cur)[r-1][c].x * K;
                if (r < row-1)  temp[r][c].a -= (*cur)[r+1][c].x * K;
                if (c > 0)      temp[r][c].a -= (*cur)[r][c-1].x * K;
                if (c < col-1)  temp[r][c].a -= (*cur)[r][c+1].x * K;
                temp[r][c].a /= 2; //Average the x- and y-components.
                temp[r][c].a -= K * (*cur)[r][c].x;
                temp[r][c].a += DAMP * (*cur)[r][c].v;
                //maintain position and velocity until it is recalculated in the sim() function
            }
        }
        //update cur
        for(size_t r = 0; r < row; r++){
            for(size_t c = 0; c < col; c++){
                (*cur)[r][c] = temp[r][c];
            }
        }

    }
    free_grid(temp, row);
    temp = NULL;
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
    for(int r = 0; r < row; r++){
        for(int c = 0; c < col; c++){
            move(r, c * 2);
            int ch_val = MIN((int)(abs((float)len * field[r][c].x / (float)MAXDISP)), len - 1);
            ch_val = MAX(0, ch_val);
            ch = GREYSCALE[ch_val];
            addch(ch);
            move(r, (2 * c) + 1);
            addch(ch);
        }
    }
    refresh();
}

void puddle(float intensity){
    int persecond = 1000000;
    int framerate = 20;
    int frameperiod = persecond / framerate;
    spring** field = new_grid(HEIGHT, WIDTH);
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
            field[y][x].x = MAXDISP;
        }
        printframe(field, HEIGHT, WIDTH);
        simulate(&field, HEIGHT, WIDTH, 1.0 / (double) framerate);
        usleep(frameperiod);
        count++;
    }
    free_grid(field, HEIGHT);
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
