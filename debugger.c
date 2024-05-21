#include "uart.h"
#include "printf.h"
#include "debugger.h"
#include "gl2.h"
#include "timer.h"
#include "gpio.h"
#include "gpio_extra.h"
#include "gpio_interrupts.h"
#include "assert.h"
#include "malloc.h"
#include "ringbuffer.h"

// Screen dimensions
const int WIDTH = 640;
const int HEIGHT = 512;

#define BUTTON GPIO_PIN17
#define FRAMERATE 30
#define TRIGGER_DELAY 1000 * 1000
#define SCREEN_FLASH_DELAY 100 * 1000

#define BUG_WIDTH 32
#define TOP_BORDER 70

typedef struct {
    bug_type type;
    
    double x;
    double y;
    double vel_x;
    double vel_y;
    int movement_mode;
    unsigned int delay_start;
    unsigned int delay_time;
    rb_t* locations;

} bug_obj;

// Pseudo random number generator, generates int from 0 to n - 1
int random(int n) {
    unsigned int x = timer_get_ticks() + 2147483648;
    x = ((x >> 17) ^ x) * 16547387;
    x = (x >> 9) ^ (x % 138);
    return x % n;
}

// Initialize a bug object and put it somewhere on the screen
bug_obj* init_bug(bug_type type) {
    bug_obj *bug = malloc(sizeof(bug_obj));
    bug->locations = rb_new();
    rb_enqueue(bug->locations, bug->x);
    rb_enqueue(bug->locations, bug->y);
    rb_enqueue(bug->locations, bug->x);
    rb_enqueue(bug->locations, bug->y);

    bug->x = BUG_WIDTH/2 + random(WIDTH - BUG_WIDTH);
    bug->y = BUG_WIDTH/2 + TOP_BORDER + random(HEIGHT - BUG_WIDTH - TOP_BORDER);

    switch(type) {
        case ANT: 
            bug->vel_x = (double)(random(301)) / 15 + 1;
            bug->vel_y = (double)(random(301)) / 15 + 1;

            bug->vel_x *= random(2) ? 1 : -1;
            bug->vel_y *= random(2) ? 1 : -1;
        case FLY: 
            bug->vel_x = (double)(random(301)) / 15 + 1;
            bug->vel_y = (double)(random(301)) / 15 + 1;

            bug->vel_x *= random(2) ? 1 : -1;
            bug->vel_y *= random(2) ? 1 : -1;
        break;
    }
    return bug;
}


// Draw a bug of the given type at the given coordinates
void draw_bug(bug_obj *bug) {

    // Cover previous bug
    int last_x;
    int last_y;
    rb_dequeue(bug->locations, &last_x);
    rb_dequeue(bug->locations, &last_y);
    gl_draw_rect(last_x / 1000 - BUG_WIDTH / 2, last_y / 1000 - BUG_WIDTH / 2, BUG_WIDTH, BUG_WIDTH, GL_BLACK);

    switch(bug->type) {
        case ANT:
            gl_draw_rect((int)bug->x - BUG_WIDTH / 2, (int)bug->y - BUG_WIDTH / 2, BUG_WIDTH, BUG_WIDTH, GL_GREEN);
            break;
        case FLY:
            gl_draw_rect((int)bug->x - BUG_WIDTH / 2, (int)bug->y - BUG_WIDTH / 2, BUG_WIDTH, BUG_WIDTH, GL_INDIGO);
            break;
    }

    rb_enqueue(bug->locations, (int)(bug->x * 1000));
    rb_enqueue(bug->locations, (int)(bug->y * 1000));
}

void draw_text(const char* buf) {
    gl_draw_string(10, 10, buf, GL_WHITE);
}

// Move the bug according to it's parameters
void move_bug(bug_obj *bug) {
    switch(bug->type) {
        case ANT:
            bug->x += bug->vel_x;
            bug->y += bug->vel_y;
            if (bug->x < BUG_WIDTH / 2 || bug->x > WIDTH - BUG_WIDTH / 2) {
                bug->vel_x *= -1;
            }
            if (bug->y < BUG_WIDTH / 2 + TOP_BORDER || bug->y > HEIGHT - BUG_WIDTH / 2) {
                bug->vel_y *= -1;
            }
        break;
        case FLY:
        break;
    }
}

volatile unsigned int last_press;

void button_press(unsigned int pc, bug_obj **b) {
    bug_obj* bug = *b;
    if (timer_get_ticks() > last_press + TRIGGER_DELAY) {
        unsigned int start_tick = timer_get_ticks();
        gl_draw_interrupt_square(bug->x, bug->y, BUG_WIDTH, GL_WHITE);
        gl_swap_interrupt_buffer();
        while (start_tick + SCREEN_FLASH_DELAY > timer_get_ticks()) {
            continue;
        }
        gl_draw_interrupt_square(bug->x, bug->y, BUG_WIDTH, GL_BLACK);

        gl_swap_buffer();
        gl_clear(GL_BLACK);
        gl_swap_buffer();
        gl_clear(GL_BLACK);


        last_press = timer_get_ticks();
    }
    gpio_clear_event(BUTTON);
}

unsigned int last_tick;

void run(void) {
    // Initializations
    interrupts_init();
    gpio_init();
    timer_init();
    uart_init();

    bug_obj *bug = init_bug(ANT);

    // Configure interrupts
    gpio_interrupts_init();
    gpio_enable_event_detection(BUTTON, GPIO_DETECT_FALLING_EDGE);
    gpio_interrupts_register_handler(BUTTON, (void*)button_press, &bug);
    gpio_interrupts_enable();
    interrupts_global_enable();

    // Clear interrupt buffer
    gl_init(WIDTH, HEIGHT);
    gl_interrupt_clear(GL_BLACK);
    gl_clear(GL_BLACK);
    bug = init_bug(ANT);
    draw_bug(bug);

    gl_swap_buffer();
    gl_clear(GL_BLACK);
    
    // Game loop
    while (1) {
        if (timer_get_ticks() / (1000000 / FRAMERATE) > last_tick / (1000000 / FRAMERATE)) {
            move_bug(bug);
            draw_bug(bug);
            draw_text("Test");
            gl_swap_buffer();
            last_tick = timer_get_ticks();
        }
    }
    
}
