#pragma once

/* CE toolchain v15 standard headers */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* CE-specific headers */
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>

/* ── screen (TI-84 Plus CE: 320×240) ── */
#define SCR_W  320
#define SCR_H  240

/* ── colors: use gfx_SetColor index values ──
   We use a fixed 8-color palette loaded at startup.
   Index 0-7 are ours; rest is the default OS palette. */
#define COL_BG      255   /* white  */
#define COL_FG      0     /* black  */
#define COL_NAVY    1     /* header/selected bg */
#define COL_BLUE    2     /* highlight */
#define COL_DKGRAY  3     /* softkey bar */
#define COL_ACCENT  4     /* label accent */
#define COL_RED     5     /* errors */
#define COL_GRAY    6     /* muted */

/* ── tuning ── */
#define SIMP_N      20    /* Simpson 2-D steps — must be even */
#define SIMP_N3     10    /* Simpson 3-D steps */
#define DIFF_H      1e-5  /* numerical diff step */
#define MAX_EXPR    40    /* max expression length */
#define MAX_FIELDS  11
#define MAX_RES      6

/* ── operation IDs ── */
typedef enum {
    OP_CURLDIV   = 0,
    OP_SURFAREA  = 1,
    OP_SURFINT   = 2,
    OP_FLUX      = 3,
    OP_VOLUME    = 4,
    OP_TRIPLE    = 5,
    OP_TANGPLANE = 6,
    OP_COUNT     = 7
} Op;

/* ── coordinate systems ── */
typedef enum {
    COORD_CART = 0,
    COORD_CYL  = 1,
    COORD_SPH  = 2
} Coords;

/* ── screen IDs ── */
typedef enum {
    SCR_MENU   = 0,
    SCR_INPUT  = 1,
    SCR_RESULT = 2,
    SCR_ORIENT = 3,
    SCR_COORDS = 4
} Screen;

/* ── one input field ── */
typedef struct {
    char label[14];
    char val[MAX_EXPR];
    char def[MAX_EXPR];
} Field;

/* ── one result line ── */
typedef struct {
    char label[24];
    char value[52];
} ResLine;

/* ── global state ── */
typedef struct {
    Screen  screen;
    Op      op;
    int     menu_cur;
    int     field_cur;
    int     orient;        /* +1 upward/outward  -1 downward/inward */
    Coords  coords;
    Field   f[MAX_FIELDS];
    int     nf;
    ResLine res[MAX_RES];
    int     nres;
    char    err[72];
    double  raw;
} State;

extern State G;

/* ── math constants ── */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── function prototypes ── */

/* eval.c */
double cas_eval(const char *expr, double x, double y, double z);
double cas_eval_lim(const char *s);

/* fields.c */
void fields_load(Op op, Coords coords);
void fields_defaults(void);

/* compute.c */
void compute(void);

/* ui.c */
void ui_draw(void);
void ui_key(kb_lkey_t row1, kb_lkey_t row2, kb_lkey_t row3,
            kb_lkey_t row4, kb_lkey_t row5, kb_lkey_t row6,
            kb_lkey_t row7);

/* ── inline helpers ── */
static inline double nd_x(const char *e, double x, double y, double z)
{ return (cas_eval(e,x+DIFF_H,y,z) - cas_eval(e,x-DIFF_H,y,z)) / (2.0*DIFF_H); }
static inline double nd_y(const char *e, double x, double y, double z)
{ return (cas_eval(e,x,y+DIFF_H,z) - cas_eval(e,x,y-DIFF_H,z)) / (2.0*DIFF_H); }
static inline double nd_z(const char *e, double x, double y, double z)
{ return (cas_eval(e,x,y,z+DIFF_H) - cas_eval(e,x,y,z-DIFF_H)) / (2.0*DIFF_H); }
static inline double nd_u(const char *e, double u, double v)
{ return (cas_eval(e,u+DIFF_H,v,0) - cas_eval(e,u-DIFF_H,v,0)) / (2.0*DIFF_H); }
static inline double nd_v(const char *e, double u, double v)
{ return (cas_eval(e,u,v+DIFF_H,0) - cas_eval(e,u,v-DIFF_H,0)) / (2.0*DIFF_H); }

static inline double cross_mag(double ax, double ay, double az,
                                double bx, double by, double bz) {
    double cx = ay*bz - az*by;
    double cy = az*bx - ax*bz;
    double cz = ax*by - ay*bx;
    return sqrt(cx*cx + cy*cy + cz*cz);
}

static inline void cross3(double ax, double ay, double az,
                           double bx, double by, double bz,
                           double *cx, double *cy, double *cz) {
    *cx = ay*bz - az*by;
    *cy = az*bx - ax*bz;
    *cz = ax*by - ay*bx;
}

static inline void fmt_double(char *buf, int n, double v) {
    double r = round(v * 1e6) / 1e6;
    long long ri = (long long)r;
    if ((double)ri == r && r > -1e9 && r < 1e9)
        snprintf(buf, n, "%lld", ri);
    else
        snprintf(buf, n, "%.5g", v);
}

static inline void add_res(const char *lbl, const char *val) {
    if (G.nres >= MAX_RES) return;
    strncpy(G.res[G.nres].label, lbl, 23);
    strncpy(G.res[G.nres].value, val, 51);
    G.nres++;
}
