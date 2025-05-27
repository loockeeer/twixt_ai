#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cairo.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include"twixt.h"
#include"montecarlo.h"


#include <cairo.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

#define CELL_SIZE 35
#define PEG_RADIUS 9
#define HOLE_RADIUS 5
#define BORDER_OFFSET 1
#define BOARD_SIZE 10  // Example board size, adjust as needed

tree_t *tree;
board_t *board;

int cursor_x;
int cursor_y;
bool is_cursor_somewhere = false;

static void draw_circle(cairo_t *cr, double x, double y, double r, double red, double green, double blue) {
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_arc(cr, x, y, r, 0, 2 * M_PI);
    cairo_fill(cr);
}

static void draw_line(cairo_t *cr, double x1, double y1, double x2, double y2, double red, double green, double blue) {
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, 4);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

position_t deltass[8] = {{1, -2}, {2, -1}, {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2}};

static void draw_board(cairo_t *cr) {
    int screen_size = (BOARD_SIZE + 2) * CELL_SIZE;

    // Background
    cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);  // #C0C0C0
    cairo_paint(cr);

    // Draw separation bars
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 5);
    cairo_move_to(cr, BORDER_OFFSET * CELL_SIZE, 0);
    cairo_line_to(cr, BORDER_OFFSET * CELL_SIZE, screen_size);
    cairo_stroke(cr);

    cairo_move_to(cr, screen_size - (2 * CELL_SIZE), 0);
    cairo_line_to(cr, screen_size - (2 * CELL_SIZE), screen_size);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_move_to(cr, 0, BORDER_OFFSET * CELL_SIZE);
    cairo_line_to(cr, screen_size, BORDER_OFFSET * CELL_SIZE);
    cairo_stroke(cr);

    cairo_move_to(cr, 0, screen_size - (2 * CELL_SIZE));
    cairo_line_to(cr, screen_size, screen_size - (2 * CELL_SIZE));
    cairo_stroke(cr);

    // Draw holes
    cairo_set_source_rgb(cr, 0.635, 0.635, 0.635);  // #A2A2A2
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            double x = (i + BORDER_OFFSET) * CELL_SIZE;
            double y = (j + BORDER_OFFSET) * CELL_SIZE;
            draw_circle(cr, x, y, HOLE_RADIUS, 0.635, 0.635, 0.635);
        }
    }

    // Draw pegs
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            node_t node = board->data[j*BOARD_SIZE+i];
            if (node.player == RED) {
                double x = (i + BORDER_OFFSET) * CELL_SIZE;
                double y = (j + BORDER_OFFSET) * CELL_SIZE;
                draw_circle(cr, x, y, PEG_RADIUS, 1, 0, 0);
            } else if (node.player == BLACK) {
                double x = (i + BORDER_OFFSET) * CELL_SIZE;
                double y = (j + BORDER_OFFSET) * CELL_SIZE;
                draw_circle(cr, x, y, PEG_RADIUS, 0, 0, 0);
            }
        }
    }
    if (is_cursor_somewhere && cursor_x != 0 && cursor_x != BOARD_SIZE-1 && board->data[cursor_y * BOARD_SIZE + cursor_x].player == NONE) {
        cairo_set_source_rgba(cr, 1, 0, 0, 0.3);
        cairo_arc(cr, (cursor_x + BORDER_OFFSET)*CELL_SIZE, (cursor_y + BORDER_OFFSET)*CELL_SIZE, PEG_RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);
        for (int i = 0; i < 8; i ++) {
            if (cursor_x +deltass[i].x < BOARD_SIZE && cursor_x + deltass[i].x >= 0
                && cursor_y +deltass[i].y < BOARD_SIZE && cursor_y + deltass[i].y >= 0
                && !(board->data[cursor_y * BOARD_SIZE + cursor_x].links & (1 << i))
                && board->data[(cursor_y+ deltass[i].y)* BOARD_SIZE + deltass[i].x + cursor_x].player == RED) {
                cairo_set_line_width(cr, 3);
                cairo_set_source_rgba(cr, 1, 0, 0, 0.3);
                cairo_move_to(cr, (cursor_x + BORDER_OFFSET) * CELL_SIZE, (cursor_y + BORDER_OFFSET) * CELL_SIZE);
                cairo_line_to(cr, (cursor_x + deltass[i].x + BORDER_OFFSET)*CELL_SIZE, (cursor_y + deltass[i].y + BORDER_OFFSET)*CELL_SIZE);
                cairo_stroke(cr);
            }
        }
    }

    bool visited[BOARD_SIZE][BOARD_SIZE];
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            visited[i][j] = false;
        }
    }
    for (int i = 0; i < BOARD_SIZE; i ++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            node_t node = board->data[j * BOARD_SIZE + i];
            for (int k = 0; k < 8; k ++) {
                if ((node.links & (1 << k)) != 0 && !visited[i + deltass[k].x][j + deltass[k].y]) {
                    cairo_set_line_width(cr, 3);
                    cairo_set_source_rgb(cr, node.player == RED, 0, 0);
                    cairo_move_to(cr, (i + BORDER_OFFSET) * CELL_SIZE, (j + BORDER_OFFSET) * CELL_SIZE);
                    cairo_line_to(cr, (i + deltass[k].x + BORDER_OFFSET)*CELL_SIZE, (j + deltass[k].y + BORDER_OFFSET)*CELL_SIZE);
                    cairo_stroke(cr);
                }
            }
            visited[i][j] = true;
        }
    }
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    draw_board(cr);
    return FALSE;
}

int distance(int x, int y, int u, int v) {
    return (x-u)*(x-u) + (y-v)*(y-v);
}

static gboolean on_mouse_click(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    printf("clicked : %d,%d\n", (int)event->x, (int)event->y);
    for (int i=0; i<BOARD_SIZE; i++) {
        for (int j=0; j<BOARD_SIZE; j++) {
            int x = (i + BORDER_OFFSET) * CELL_SIZE;
            int y = (j + BORDER_OFFSET) * CELL_SIZE;
            if (distance(x,y,(int)event->x, (int)event->y) <= PEG_RADIUS*PEG_RADIUS) {
                if (twixt_play(board, RED, (position_t){i, j})) {
                    outcome_t o = twixt_check_winner(board);
                    if (o != ONGOING) {
                        if (o == RED_WINS) printf("red\n");
                        if (o == BLACK_WINS) printf("black\n");
                        if (o == DRAW) printf("draw\n");
                        exit(1);
                    }
                    mc_advance_tree(&tree, (position_t){i, j});
                    position_t move = mc_search(500, 500, board, &tree, BLACK);
                    twixt_play(board, BLACK, move);
                    o = twixt_check_winner(board);
                    if (o != ONGOING) {
                        if (o == RED_WINS) printf("red\n");
                        if (o == BLACK_WINS) printf("black\n");
                        if (o == DRAW) printf("draw\n");
                        exit(1);
                    }
                }
                break;
            }
        }
    }
    gtk_widget_queue_draw(widget);

    return TRUE;
}

static gboolean on_mouse_hover(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    is_cursor_somewhere = false;
    gtk_widget_queue_draw(widget);

    for (int i=0; i<BOARD_SIZE; i++) {
        for (int j=0; j<BOARD_SIZE; j++) {
            int x = (i + BORDER_OFFSET) * CELL_SIZE;
            int y = (j + BORDER_OFFSET) * CELL_SIZE;
            if (distance(x,y,(int)event->x, (int)event->y) <= PEG_RADIUS*PEG_RADIUS) {
                is_cursor_somewhere = true;
                cursor_x = i;
                cursor_y = j;
                gtk_widget_queue_draw(widget);
                return TRUE;
            }
        }
    }
    is_cursor_somewhere = false;
    return TRUE;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    gtk_init(&argc, &argv);
    tree = mc_create_tree();
    board = twixt_create(BOARD_SIZE);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(window), "Twixt");
    gtk_window_set_default_size(GTK_WINDOW(window), (BOARD_SIZE + 2) * CELL_SIZE, (BOARD_SIZE + 2) * CELL_SIZE);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), drawing_area);

    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_mouse_click), NULL);
    g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_mouse_hover), NULL);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
/*
int main(void) {
    srand(time(NULL));
    tree_t *tree = mc_create_tree();
    board_t *game = twixt_create(15);
    char buffer[100];
    int x, y;
    /*mc_advance_tree(&tree, (position_t){12,12});
    position_t move = mc_search(20, 1, game, &tree, BLACK);
    printf("-> (%d, %d)\n", move.x, move.y);
    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (sscanf(buffer, "%d,%d", &x, &y) == 2) {
            printf("Searching...\n");
            mc_advance_tree(&tree, (position_t){x, y});
            position_t move = mc_search(600, 200, game, &tree, BLACK);
            printf("-> (%d, %d)\n", move.x, move.y);
        } else {
            printf("Invalid input. Please enter in 'number,number' format.\n");
        }
    }

    return 0;
}*/