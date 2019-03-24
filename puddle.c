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
double K = 44.5;
double MAXDISP = 100;

static const char GREYSCALE[] = " .:-=+*#%@";

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

//Propagate foces to neighboring springs
void prop(spring*** cur, spring*** prev, size_t row, size_t col){
    spring** temp = new_grid(row, col);
    for(int r = 0; r < row; r++){
        for(int c = 0; c < col; c++){
            //add up all forces (equivalent to acceleration with m=1)
            if (r > 0)      temp[r][c].a += (*cur)[r-1][c].x * K * -1;
            if (r < row-1)  temp[r][c].a += (*cur)[r+1][c].x * K * -1;
            if (c > 0)      temp[r][c].a += (*cur)[r][c-1].x * K * -1;
            if (c < col-1)  temp[r][c].a += (*cur)[r][c+1].x * K * -1;
            temp[r][c].a /= 2; //Average the x- and y-components.
            temp[r][c].a -= K * (*cur)[r][c].x;
            temp[r][c].a -= (*prev)[r][c].a;
            temp[r][c].a *= DAMP;
            //maintain position and velocity until it is recalculated in the sim() function
            temp[r][c].x = (*cur)[r][c].x;
            temp[r][c].v = (*cur)[r][c].v;
        }
    }
    //update prev and cur; free temp
    for(size_t r = 0; r < row; r++){
        for(size_t c = 0; c < col; c++){
            (*prev)[r][c] = (*cur)[r][c];
            (*cur)[r][c] = temp[r][c];
        }
    }
    free_grid(temp, row);
    temp = NULL;
}

//Run physics simulation on springs
void sim(spring*** grid, size_t row, size_t col){
    double dt = 0.10;
    for(size_t r = 0; r < row; r++){
        for(size_t c = 0; c < col; c++){
            (*grid)[r][c].v = (dt * (*grid)[r][c].a);
            (*grid)[r][c].x = (dt * (*grid)[r][c].v) + (dt * dt * (*grid)[r][c].a)/2;
        }
    }
}

//-------------------------------------------[Rendering]--------------------------------------------

void start_ncurses(){
    initscr();
    start_color();
    cbreak();
    curs_set(0); //Invisible cursor
    timeout(0); //make getch() nonblocking
    keypad(stdscr, TRUE);
    getmaxyx(stdscr, HEIGHT, WIDTH);
}

void printframe(spring** field, size_t row, size_t col){
    char ch;
    int len = strlen(GREYSCALE);
    for(int r = 0; r < row; r++){
        for(int c = 0; c < col; c++){
            move(r, c);
            int ch_val = MIN((int)(abs((float)len * field[r][c].x / (float)MAXDISP)), len - 1);
            ch_val = MAX(0, ch_val);
            ch = GREYSCALE[ch_val];
            addch(ch);
        }
    }
    refresh();
}

void puddle(){
    int persecond = 1000000;
    int framerate = 20;
    int frameperiod = persecond / framerate;
    spring** field = new_grid(HEIGHT, WIDTH);
    spring** oldfield = new_grid(HEIGHT, WIDTH);
    int c = 0;
    int x = rand() % WIDTH;
    int y = rand() % HEIGHT;
    int wait = rand() % (framerate * 4);
    int count = 0;
    field[y][x].x = MAXDISP;
    prop(&field, &oldfield, HEIGHT, WIDTH);
    while ((c = getch()) != 'q'){
        if (count == wait){
            x = rand() % WIDTH;
            y = rand() % HEIGHT;
            wait = rand() % (framerate * 4);
            count = 0;
            field[y][x].x = 9 * MAXDISP;
        }
        printframe(field, HEIGHT, WIDTH);
        prop(&field, &oldfield, HEIGHT, WIDTH);
        sim(&field, HEIGHT, WIDTH);
        usleep(frameperiod);
        count++;
    }
    free_grid(field, HEIGHT);
    free_grid(oldfield, HEIGHT);
    field = NULL;
    oldfield = NULL;
    clear();
    refresh();
    endwin();
}

int main(int argc, char** argv){
    srand(time(NULL));
    start_ncurses();
    puddle();
    return 0;
}
