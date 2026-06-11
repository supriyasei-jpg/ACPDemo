/*
 * 2D Graphics Editor in C
 * Uses a 2D character array as canvas.
 * Shapes drawn using '*' (border) and '_' (fill / background).
 * Supports: Circle, Rectangle, Line, Triangle
 * Operations: Add, Delete, Modify objects
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ─── Canvas Configuration ─────────────────────────────────────── */
#define ROWS    25
#define COLS    60
#define EMPTY   '_'
#define DRAW    '*'
#define MAX_OBJ 50

/* ─── Shape Types ───────────────────────────────────────────────── */
typedef enum {
    CIRCLE    = 1,
    RECTANGLE = 2,
    LINE      = 3,
    TRIANGLE  = 4
} ShapeType;

/* ─── Object Descriptor ─────────────────────────────────────────── */
typedef struct {
    int       id;
    ShapeType type;
    /* For Circle:    x=cx, y=cy, w=radius                          */
    /* For Rectangle: x=col, y=row, w=width, h=height               */
    /* For Line:      x=x1, y=y1, w=x2, h=y2                        */
    /* For Triangle:  x=tip_col, y=tip_row, w=base_width            */
    int x, y, w, h;
    int active;   /* 1 = present on canvas, 0 = deleted             */
} Object;

/* ─── Globals ───────────────────────────────────────────────────── */
char   canvas[ROWS][COLS];
Object objects[MAX_OBJ];
int    obj_count = 0;
int    next_id   = 1;

/* ══════════════════════════════════════════════════════════════════
 *  Canvas helpers
 * ══════════════════════════════════════════════════════════════════ */

void init_canvas(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            canvas[r][c] = EMPTY;
}

/* Safe pixel setter */
void set_pixel(int row, int col, char ch) {
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        canvas[row][col] = ch;
}

void display_canvas(void) {
    printf("\n");
    /* Column ruler */
    printf("   ");
    for (int c = 0; c < COLS; c++) printf("%d", c % 10);
    printf("\n");

    for (int r = 0; r < ROWS; r++) {
        printf("%2d ", r);
        for (int c = 0; c < COLS; c++)
            putchar(canvas[r][c]);
        putchar('\n');
    }
    printf("\n");
}

/* ══════════════════════════════════════════════════════════════════
 *  Drawing primitives  (Bresenham / midpoint algorithms)
 * ══════════════════════════════════════════════════════════════════ */

/* --- Bresenham line ------------------------------------------------ */
void draw_line_pixels(int x1, int y1, int x2, int y2, char ch) {
    int dx = abs(x2 - x1), sx = (x1 < x2) ? 1 : -1;
    int dy = abs(y2 - y1), sy = (y1 < y2) ? 1 : -1;
    /* Note: x = column, y = row */
    int err = dx - dy;
    while (1) {
        set_pixel(y1, x1, ch);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

/* --- Midpoint circle ----------------------------------------------- */
void draw_circle_pixels(int cx, int cy, int r, char ch) {
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        set_pixel(cy + y, cx + x, ch);
        set_pixel(cy - y, cx + x, ch);
        set_pixel(cy + y, cx - x, ch);
        set_pixel(cy - y, cx - x, ch);
        set_pixel(cy + x, cx + y, ch);
        set_pixel(cy - x, cx + y, ch);
        set_pixel(cy + x, cx - y, ch);
        set_pixel(cy - x, cx - y, ch);
        if (d < 0)      d += 2 * x + 3;
        else          { d += 2 * (x - y) + 5; y--; }
        x++;
    }
}

/* --- Rectangle (outline) ------------------------------------------ */
void draw_rect_pixels(int col, int row, int w, int h, char ch) {
    for (int c = col; c < col + w; c++) {
        set_pixel(row,         c, ch);
        set_pixel(row + h - 1, c, ch);
    }
    for (int r = row; r < row + h; r++) {
        set_pixel(r, col,         ch);
        set_pixel(r, col + w - 1, ch);
    }
}

/* --- Triangle (isoceles, tip at top) ------------------------------ */
void draw_triangle_pixels(int tip_col, int tip_row, int base_w, char ch) {
    int half   = base_w / 2;
    int height = half + 1;            /* proportional height */
    for (int i = 0; i < height; i++) {
        int left  = tip_col - i;
        int right = tip_col + i;
        int row   = tip_row + i;
        set_pixel(row, left,  ch);
        set_pixel(row, right, ch);
    }
    /* base */
    int base_row = tip_row + height - 1;
    for (int c = tip_col - half; c <= tip_col + half; c++)
        set_pixel(base_row, c, ch);
}

/* ══════════════════════════════════════════════════════════════════
 *  Redraw all active objects onto a fresh canvas
 * ══════════════════════════════════════════════════════════════════ */
void redraw_all(void) {
    init_canvas();
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        Object *o = &objects[i];
        switch (o->type) {
            case CIRCLE:
                draw_circle_pixels(o->x, o->y, o->w, DRAW);
                break;
            case RECTANGLE:
                draw_rect_pixels(o->x, o->y, o->w, o->h, DRAW);
                break;
            case LINE:
                draw_line_pixels(o->x, o->y, o->w, o->h, DRAW);
                break;
            case TRIANGLE:
                draw_triangle_pixels(o->x, o->y, o->w, DRAW);
                break;
        }
    }
}

/* ══════════════════════════════════════════════════════════════════
 *  Object management: Add / Delete / Modify
 * ══════════════════════════════════════════════════════════════════ */

int add_object(ShapeType type, int x, int y, int w, int h) {
    if (obj_count >= MAX_OBJ) {
        printf("Canvas is full (max %d objects).\n", MAX_OBJ);
        return -1;
    }
    Object *o = &objects[obj_count++];
    o->id     = next_id++;
    o->type   = type;
    o->x      = x;
    o->y      = y;
    o->w      = w;
    o->h      = h;
    o->active = 1;
    redraw_all();
    printf("Added object ID %d.\n", o->id);
    return o->id;
}

int find_object(int id) {
    for (int i = 0; i < obj_count; i++)
        if (objects[i].id == id && objects[i].active) return i;
    return -1;
}

void delete_object(int id) {
    int idx = find_object(id);
    if (idx < 0) { printf("Object ID %d not found.\n", id); return; }
    objects[idx].active = 0;
    redraw_all();
    printf("Deleted object ID %d.\n", id);
}

void modify_object(int id, int x, int y, int w, int h) {
    int idx = find_object(id);
    if (idx < 0) { printf("Object ID %d not found.\n", id); return; }
    objects[idx].x = x;
    objects[idx].y = y;
    objects[idx].w = w;
    objects[idx].h = h;
    redraw_all();
    printf("Modified object ID %d.\n", id);
}

/* ══════════════════════════════════════════════════════════════════
 *  List objects
 * ══════════════════════════════════════════════════════════════════ */
void list_objects(void) {
    const char *names[] = {"", "Circle", "Rectangle", "Line", "Triangle"};
    printf("\n%-5s %-12s %-5s %-5s %-5s %-5s\n",
           "ID", "Shape", "X", "Y", "W", "H");
    printf("---------------------------------------\n");
    int any = 0;
    for (int i = 0; i < obj_count; i++) {
        if (!objects[i].active) continue;
        Object *o = &objects[i];
        printf("%-5d %-12s %-5d %-5d %-5d %-5d\n",
               o->id, names[o->type], o->x, o->y, o->w, o->h);
        any = 1;
    }
    if (!any) printf("  (no objects)\n");
    printf("\n");
}

/* ══════════════════════════════════════════════════════════════════
 *  Interactive menu
 * ══════════════════════════════════════════════════════════════════ */
void print_menu(void) {
    printf("╔══════════════════════════════╗\n");
    printf("║   2D Graphics Editor (C)     ║\n");
    printf("╠══════════════════════════════╣\n");
    printf("║ 1. Add Circle                ║\n");
    printf("║ 2. Add Rectangle             ║\n");
    printf("║ 3. Add Line                  ║\n");
    printf("║ 4. Add Triangle              ║\n");
    printf("║ 5. Delete Object             ║\n");
    printf("║ 6. Modify Object             ║\n");
    printf("║ 7. Display Canvas            ║\n");
    printf("║ 8. List Objects              ║\n");
    printf("║ 0. Exit                      ║\n");
    printf("╚══════════════════════════════╝\n");
    printf("Choice: ");
}

int main(void) {
    int choice, id, x, y, w, h;

    init_canvas();

    /* ── Demo: pre-populate a few shapes ─────────────────────────── */
    printf("\n[Demo] Adding initial shapes...\n");
    add_object(RECTANGLE, 2,  1, 20, 10);   /* rect at col2,row1  */
    add_object(CIRCLE,   45, 10,  8,  0);   /* circle at col45,row10 r=8 */
    add_object(LINE,      2, 12, 25, 22);   /* line from (2,12) to (25,22) */
    add_object(TRIANGLE, 35,  2, 12,  0);   /* triangle tip at col35,row2 */
    display_canvas();
    printf("[Demo done — entering interactive mode]\n\n");

    while (1) {
        print_menu();
        if (scanf("%d", &choice) != 1) { scanf("%*s"); continue; }

        switch (choice) {
            case 1: /* Circle */
                printf("Enter cx cy radius: ");
                scanf("%d %d %d", &x, &y, &w);
                add_object(CIRCLE, x, y, w, 0);
                display_canvas();
                break;

            case 2: /* Rectangle */
                printf("Enter col row width height: ");
                scanf("%d %d %d %d", &x, &y, &w, &h);
                add_object(RECTANGLE, x, y, w, h);
                display_canvas();
                break;

            case 3: /* Line */
                printf("Enter x1 y1 x2 y2: ");
                scanf("%d %d %d %d", &x, &y, &w, &h);
                add_object(LINE, x, y, w, h);
                display_canvas();
                break;

            case 4: /* Triangle */
                printf("Enter tip_col tip_row base_width: ");
                scanf("%d %d %d", &x, &y, &w);
                add_object(TRIANGLE, x, y, w, 0);
                display_canvas();
                break;

            case 5: /* Delete */
                list_objects();
                printf("Enter ID to delete: ");
                scanf("%d", &id);
                delete_object(id);
                display_canvas();
                break;

            case 6: /* Modify */
                list_objects();
                printf("Enter ID to modify: ");
                scanf("%d", &id);
                {
                    int idx = find_object(id);
                    if (idx < 0) { printf("ID not found.\n"); break; }
                    switch (objects[idx].type) {
                        case CIRCLE:
                            printf("New cx cy radius: ");
                            scanf("%d %d %d", &x, &y, &w);
                            modify_object(id, x, y, w, 0);
                            break;
                        case RECTANGLE:
                            printf("New col row width height: ");
                            scanf("%d %d %d %d", &x, &y, &w, &h);
                            modify_object(id, x, y, w, h);
                            break;
                        case LINE:
                            printf("New x1 y1 x2 y2: ");
                            scanf("%d %d %d %d", &x, &y, &w, &h);
                            modify_object(id, x, y, w, h);
                            break;
                        case TRIANGLE:
                            printf("New tip_col tip_row base_width: ");
                            scanf("%d %d %d", &x, &y, &w);
                            modify_object(id, x, y, w, 0);
                            break;
                    }
                }
                display_canvas();
                break;

            case 7: /* Display */
                display_canvas();
                break;

            case 8: /* List */
                list_objects();
                break;

            case 0:
                printf("Goodbye!\n");
                return 0;

            default:
                printf("Invalid choice.\n");
        }
    }
}

/*
 * ─── Compilation & Run ──────────────────────────────────────────────
 *
 *   gcc -o graphics_editor graphics_editor.c -lm
 *   ./graphics_editor
 *
 * ─── Parameter reference ────────────────────────────────────────────
 *
 *   Circle    : cx  cy  radius          (h unused)
 *   Rectangle : col row width   height
 *   Line      : x1  y1  x2      y2      (stored in x,y,w,h)
 *   Triangle  : tip_col  tip_row  base_width  (h unused)
 *
 *   Canvas    : ROWS=25  COLS=60
 *               '*' = drawn pixel
 *               '_' = empty / background
 * ────────────────────────────────────────────────────────────────── */
