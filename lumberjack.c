/* ---------------------------------------------------------------------------- 
- IN/OUT DEVICES -

input:      KEYS            0xFF200050      - user input

output:     HEX3-0          0xFF200020      - display score
            VGA             0xFF203020      - game display

other:      Private Timer   0xFFFEC600      - turn time

- USER INPUT -

KEY0    -> man swing right 
KEY1    -> man swing left
ANY_KEY -> start game

- left is 1, right is 0 for lumberjack and branches
- check branch[0] for collision with lumberjack
-------------------------------------------------------------------------------*/

#include <stdbool.h>


volatile int pixel_buffer_start;

// pointers to IO devices
volatile int * keys_ptr;
volatile int * hex30_ptr;
volatile int * hex54_ptr;
volatile int * priv_timer_ptr;
volatile int * int_timer_ptr;

// global variables
int branch_positions[] = {2, 2, 2, 2};   // 2 = empty, 1 = left, 0 = right
int seg7[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; // bitcodes for 0-9
int total_branches = 4;
int highscore = 0;
bool game_over = false;

// images
extern short MENU[120][160];
extern short BACKGROUND[120][160];
extern short GAMEOVER[120][160];
extern short MAN_LEFT[20][65];
extern short MAN_SWING_LEFT[20][65];
extern short MAN_RIGHT[20][65];
extern short MAN_SWING_RIGHT[20][65];
extern short BRANCH_LEFT[20][65];
extern short BRANCH_RIGHT[20][65];

// function declarations
void game_startup();
void game_setup();
void game_loop();
void game_end(int score);
void display_time();
void man_swing(int man_position);
void move_branches();
int reset_timer(int score);
void draw_pixel(int x, int y, short int colour);

int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;    
    // Read location of the pixel buffer from the pixel buffer controller
    pixel_buffer_start = *pixel_ctrl_ptr;

    keys_ptr = (int *)0xFF200050;
    hex30_ptr = (int *)0xFF200020;
    hex54_ptr = (int *)0xFF200030;
    priv_timer_ptr = (int *)0xFFFEC600;
    int_timer_ptr = (int *)0xFF202000; 

    while (1){
        // menu + background
        game_startup();
        // setup 
        game_setup();
        // main game loop
        game_loop();
    }
}

void game_startup(){
    int i = 0, j = 0;

    // draw menu screen
    for (i = 0; i < 120; i++){
        for (j = 0; j < 160; j++){
           draw_pixel(i, j, MENU[i][j]);
        }
    }

    // wait for button press, display highscore while waiting
    while (*(keys_ptr + 3) == 0){
        *hex30_ptr = seg7[highscore / 100]<<16 | seg7[(highscore / 10) % 10]<<8 | seg7[highscore % 10];
        *hex54_ptr = 0;
    }

    // clear edgecaptures
    *(keys_ptr + 3) = *(keys_ptr + 3);
    *hex30_ptr = 0;
    
    // draw background
    volatile short *pixelbuf = 0xc8000000;
    for (i = 0; i < 120; i++){
        for (j = 0; j < 160; j++){
            *(pixelbuf + (j<<0) + (i<<9)) = BACKGROUND[i][j];
        }
    }
	
}

void game_setup(){
    int i, j, y;
    for (i = 0; i < 20; i++){
        for (j = 0; j < 65; j++){
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = MAN_LEFT[i][j];
                }
            }
        }
    }

    // randomize branches
    for (y = 0; y  < total_branches; y++){
        branch_positions[y] = rand() % 2;   // random 0 or 1
		
		//draw branch
		if (branch_positions[y] == 1){ // left side
			draw_pixel(50,  (total_branches - y) * 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_LEFT[i][j];
                }
            }
		} else { // right side
   			draw_pixel(200, (total_branches - y) * 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_RIGHT[i][j];
                }
            }
		}
    }

    // start timer
    *(priv_timer_ptr) = 400000000;   // 400,000,000 * 200MHz = 2 sec
    *(priv_timer_ptr + 3) = 1;       // reset F flag
    *(priv_timer_ptr + 2) = 0b001;   // I = 0, A = 0, E = 1
}

void game_loop(){
    int level = 1;
    int score = 0;
    game_over = false;

    while (!game_over){
        // wait for a key press or for the time to run out
        while ((*(keys_ptr + 3) & 0b0001) == 0 && (*(keys_ptr + 3) & 0b0010) == 0 && *(priv_timer_ptr + 3) == 0){
            *hex30_ptr = seg7[score / 100]<<16 | seg7[(score / 10) % 10]<<8 | seg7[score % 10];
            *hex54_ptr = seg7[level]<<16;
            // update time on display
            display_time();
        }
        
        // stop timer
        *(priv_timer_ptr + 2) = 0b000;   // I = 0, A = 0, E = 0
        if (*(priv_timer_ptr + 3) == 1){
            game_over = true;
        } else {
            // check which KEY was pressed
            if (*(keys_ptr + 3) >= 0b0010){ // KEY1 pressed
                man_swing(1);
                level = reset_timer(score);
            } else if (*(keys_ptr + 3) >= 0b0001){ // KEY0 pressed
                man_swing(0);
                level = reset_timer(score);
            }

            // clear edgecaptures
            *(keys_ptr + 3) = *(keys_ptr + 3);

            if (!game_over){
                score += 1;
            }
        }
    }

    game_end(score);
}

void display_time(){
    int length = *(priv_timer_ptr + 1) / (*(priv_timer_ptr) / 30);
    int i,j;
    int offset = 10;

    for (i = 0 + offset; i < length + 1 + offset; i++){
        for (j = 20 + offset; j < 28 + offset; j++){
            draw_pixel(i, j, 0x07E0);
        }
    }
    for (i = length + 1  + offset; i < 30 + offset; i++){
        for (j = 20 + offset; j < 28 + offset; j++){
            draw_pixel(i, j, 0x0);
        }
    }
}

void game_end(int score){
    int i, j;
    // display game over screen
    volatile short *pixelbuf = 0xc8000000;
    for (i = 0; i < 120; i++){
        for (j = 0; j < 160; j++){
            *(pixelbuf + (j<<0) + (i<<9)) = GAME_OVER[i][j];
        }
    }

    bool show_highscore = false;

    // wait for key press to return to menu, show highscore vs score while waiting
    while (*(keys_ptr + 3) == 0){

        *(priv_timer_ptr) = 100000000;  // 100,000,000 * 200MHz = 0.50 sec
        *(priv_timer_ptr + 3) = 1;          // reset F flag
        *(priv_timer_ptr + 2) = 0b001;       // I = 0, A = 0, E = 1    

        while (*(priv_timer_ptr + 3) == 0){
            if (show_highscore){
                *hex54_ptr = seg7[(highscore / 10) % 10] | seg7[highscore / 100]<<8;
                *hex30_ptr = seg7[highscore % 10]<<24;
            } else {
                *hex30_ptr = seg7[score / 100]<<16 | seg7[(score / 10) % 10]<<8 | seg7[score % 10];
                *hex54_ptr = 0;
            }
        }

        show_highscore = !show_highscore;
    }

    // clear edgecaptures
    *(keys_ptr + 3) = *(keys_ptr + 3);    

    // display new highscore
    if (score > highscore){
        highscore = score;
    }
}

void man_swing(int man_position){
    if (man_position == branch_positions[0]){ // check for collision with branch
        game_over = true;
    } else {	    
    	int i,  j;
 
        if (man_position == 1){
            // draw man on left
            draw_pixel(50, 220, 0xFFFFF);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    //*(pixelbuf + (j<<0) + (i<<9)) = MAN_SWING_LEFT[i][j];
                }
            }
        } else if (man_position == 0){
            // draw man on right
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = MAN_SWING_RIGHT[i][j];
                }
            }
        }

        // draw new branch positions
        move_branches();

        // short delay for swing animation
        *(int_timer_ptr + 2) = 0x9680;
        *(int_timer_ptr + 3) = 0x0098;  // 10,000,000 * 100MHz = 0.1 sec
        *(int_timer_ptr) = 0x0;         // reset TO flag
        *(int_timer_ptr + 1) = 0b0100;  // STOP = 0, START = 1, CONT = 0, ITO = 0

        while (*(int_timer_ptr) & 0b01 == 0){
        }

        if (man_position == 1){ // man on left
            // draw man on left
            draw_pixel(50, 220, 0xFFFFF);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = MAN_LEFT[i][j];
                }
            }
        } else if (man_position == 0){ // man on right
            // draw man on right
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = MAN_RIGHT[i][j];
                }
            }
        }
    }
}

void move_branches(){
    int i, j, y;

    // move branches down
    for (y = 0; y < total_branches - 1; y++){
        branch_positions[y] = branch_positions[y + 1];
		if (branch_positions[y] == 0){ // right side
			draw_pixel(200, (total_branches - y) * 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_RIGHT[i][j];
                }
            }
		} else if (branch_positions[y] == 1){
			draw_pixel(50,  (total_branches - y) * 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_LEFT[i][j];
                }
            }
		}
    }

    // create new random branch at the top 
    branch_positions[total_branches - 1] = rand() % 2;  // random 0 or 1
	
	//draw last branch
	if (branch_positions[total_branches - 1] == 0){ // right side
		draw_pixel(200, 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_RIGHT[i][j];
                }
            }
	} else if (branch_positions[total_branches - 1] == 1){ //left side
		draw_pixel(50, 25, 0x07E0);
            volatile short *pixelbuf = 0xc8000000;
            for (i = 0; i < 20; i++){
                for (j = 0; j < 65; j++){
                    *(pixelbuf + (j<<0) + (i<<9)) = BRANCH_LEFT[i][j];
                }
            }
	}
}

int reset_timer(int score){
    int level;
    if (score >= 100){
        *(priv_timer_ptr) = 50000000;   // 50,000,000 * 200MHz = 0.25 sec
        level = 5;
    } else if (score >= 75){
        *(priv_timer_ptr) = 100000000;  // 100,000,000 * 200MHz = 0.50 sec
        level = 4;
    } else if (score >= 50){
        *(priv_timer_ptr) = 200000000;  // 200,000,000 * 200MHz = 1.00 sec
        level = 3;
    } else if (score >= 25){
        *(priv_timer_ptr) = 300000000;  // 300,000,000 * 200MHz = 1.50 sec
        level = 2;
    } else {
        *(priv_timer_ptr) = 400000000;  // 400,000,000 * 200MHz = 2.00 sec
        level = 1;
    }

    *(priv_timer_ptr + 3) = 1;          // reset F flag
    *(priv_timer_ptr + 2) = 0b011;      // I = 0, A = 1, E = 1

    return level;
}

void draw_pixel(int x, int y, short int colour){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = colour;
}   
