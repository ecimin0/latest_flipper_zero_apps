#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <counter_icons.h>
#include <furi_hal_random.h>
#include <string.h>

#define MAX_COUNT SHRT_MAX
#define MIN_COUNT SHRT_MIN
#define BOXTIME 2
#define BOXWIDTH 30
#define MIDDLE_X 64 - BOXWIDTH / 2 // 49
#define MIDDLE_Y 32 - BOXWIDTH / 2 // 17
// #define OFFSET_Y 9
#define OFFSET_Y 6

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    FuriMutex** mutex;
    int16_t count;
    bool pressed;
    int boxtimer;
    bool needs_binary;
    bool skuldug;
} Counter;

void state_free(Counter* c) {
    gui_remove_view_port(c->gui, c->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(c->view_port);
    furi_message_queue_free(c->input_queue);
    furi_mutex_free(c->mutex);
    free(c);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    Counter* c = ctx;
    if(input_event->type == InputTypeShort) {
        furi_message_queue_put(c->input_queue, input_event, 0);
    } else if(input_event->type == InputTypeLong) {
        furi_message_queue_put(c->input_queue, input_event, 0);
    }
}

void int_to_binary(char* buf, int16_t n) {
    int16_t num = n;
    for (int16_t i = 15; i >= 0; i--) {
        //printf("%d %d %d\n", i, 16 - i, num);
        buf[i] = (num & 1) + '0';
        num >>= 1;
        //printf("%c\n", buf[16 - i]);
        //printf("%d\n", num);
    }
    buf[16] = '\0';
		printf("%s\n", buf);
}

static void render_callback(Canvas* canvas, void* ctx) {
    Counter* c = ctx;
    furi_check(furi_mutex_acquire(c->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    if (c->skuldug) {
        canvas_draw_rbox(canvas, 0, 0, 128, 64, 10);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignCenter, "Skulduggery!");
    } else {
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignCenter, "Counter");
    }
    canvas_set_font(canvas, FontBigNumbers);
    // canvas_set_font(canvas, FontSecondary);
    char scount[17];
    // if(c->pressed == true || c->boxtimer > 0) {
    //     // canvas_draw_rframe(canvas, MIDDLE_X, MIDDLE_Y + OFFSET_Y, BOXWIDTH * 2, BOXWIDTH, 5);
    //     // canvas_draw_rframe(
    //     //     canvas, MIDDLE_X - 1, MIDDLE_Y + OFFSET_Y - 1, BOXWIDTH * 2 + 2, BOXWIDTH + 2, 5);
    //     // canvas_draw_rframe(
    //     //     canvas, MIDDLE_X - 2, MIDDLE_Y + OFFSET_Y - 2, BOXWIDTH * 2 + 4, BOXWIDTH + 4, 5);
    //     canvas_draw_rframe(canvas, 34, MIDDLE_Y + OFFSET_Y, BOXWIDTH * 2, BOXWIDTH, 5);
    //     canvas_draw_rframe(
    //         canvas, 34 - 1, MIDDLE_Y + OFFSET_Y - 1, BOXWIDTH * 2 + 2, BOXWIDTH + 2, 5);
    //     canvas_draw_rframe(
    //         canvas, 34 - 2, MIDDLE_Y + OFFSET_Y - 2, BOXWIDTH * 2 + 4, BOXWIDTH + 4, 5);
    //     c->pressed = false;
    //     c->boxtimer--;
    // } else {
    //     // canvas_draw_rframe(canvas, MIDDLE_X, MIDDLE_Y + OFFSET_Y, BOXWIDTH, BOXWIDTH, 10);
    //     canvas_draw_rframe(canvas, 34, MIDDLE_Y + OFFSET_Y, BOXWIDTH * 2, BOXWIDTH, 10);
    // }
    // snprintf(scount, sizeof(scount), "%d", c->count);
    if (!c->needs_binary) {
        canvas_set_font(canvas, FontBigNumbers);
        snprintf(scount, 7, "%d", c->count);
    } else {
        canvas_set_font(canvas, FontSecondary);
        int_to_binary(scount, c->count);
    }

    canvas_draw_str_aligned(canvas, 64, 32 + OFFSET_Y, AlignCenter, AlignCenter, scount);
    furi_mutex_release(c->mutex);
}

Counter* state_init() {
    Counter* c = malloc(sizeof(Counter));
    c->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    c->view_port = view_port_alloc();
    // view_port_set_width(c->view_port, 144);
    // view_port_update(c->view_port);
    c->gui = furi_record_open(RECORD_GUI);
    c->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    c->count = 0;
    c->boxtimer = 0;
    c->needs_binary = false;
    c->skuldug = false;
    view_port_input_callback_set(c->view_port, input_callback, c);
    view_port_draw_callback_set(c->view_port, render_callback, c);
    gui_add_view_port(c->gui, c->view_port, GuiLayerFullscreen);
    return c;
}

int32_t counterapp(void) {
    Counter* c = state_init();
    while(1) {
        InputEvent input;
        while(furi_message_queue_get(c->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            furi_check(furi_mutex_acquire(c->mutex, FuriWaitForever) == FuriStatusOk);
            if(input.key == InputKeyBack) {
                furi_mutex_release(c->mutex);
                state_free(c);
                return 0;
            } else if(input.key == InputKeyUp && c->count < MAX_COUNT && input.type == InputTypeShort) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count++;
            } else if(input.key == InputKeyUp && c->count < MAX_COUNT && input.type == InputTypeLong) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count += 100;
            // } else if(input.key == InputKeyDown && c->count != 0) {
            } else if(input.key == InputKeyDown && c->count > MIN_COUNT && input.type == InputTypeShort) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count--;
            } else if(input.key == InputKeyDown && c->count > MIN_COUNT && input.type == InputTypeLong) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count -= 100;
            } else if(input.key == InputKeyOk && input.type == InputTypeShort) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->count = 0;
            } else if(input.key == InputKeyOk && input.type == InputTypeLong && !c->skuldug) {
                c->skuldug = true;
                c->pressed = true;
                c->boxtimer = BOXTIME;
                // c->count = 0;
            } else if(input.key == InputKeyOk && input.type == InputTypeLong && c->skuldug) {
                c->skuldug = false;
                c->pressed = true;
                c->boxtimer = BOXTIME;
            } else if (input.key == InputKeyLeft) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                c->needs_binary = !c->needs_binary;
            } else if (input.key == InputKeyRight) {
                c->pressed = true;
                c->boxtimer = BOXTIME;
                int16_t r = furi_hal_random_get();
                c->count = r;
            }
            furi_mutex_release(c->mutex);
            view_port_update(c->view_port);
        }
    }
    state_free(c);
    return 0;
}
