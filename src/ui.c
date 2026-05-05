#include "cas.h"

/* TI-84 Plus CE: 320x240, 8x8 monospaced font with gfx_PrintStringXY
   Layout:
     0-17   : header bar
     18-206 : body (up to 23 rows at 8px each)
     207-223: softkey bar
*/

#define CHAR_H    8
#define BODY_TOP  20
#define SOFT_Y    207
#define VISIBLE   7    /* max fields shown at once */

static const char *OP_NAMES[OP_COUNT] = {
    "CURL & DIV",
    "SURFACE AREA",
    "SURF INTEGRAL",
    "FLUX INTEGRAL",
    "VOLUME",
    "TRIPLE INTEGRAL",
    "TANGENT PLANE"
};

/* ── low-level helpers ── */
static void fill(int x, int y, int w, int h, uint8_t c) {
    gfx_SetColor(c);
    gfx_FillRectangle(x, y, w, h);
}

static void txt(int x, int y, uint8_t fg, uint8_t bg, const char *s) {
    gfx_SetTextFGColor(fg);
    gfx_SetTextBGColor(bg);
    gfx_PrintStringXY(s, x, y);
}

static void hdr(const char *title) {
    fill(0, 0, SCR_W, 18, COL_NAVY);
    txt(4, 5, 255, COL_NAVY, title);
}

static void softbar(const char *f1, const char *f2, const char *f3,
                    const char *f4, const char *f5, const char *f6) {
    fill(0, SOFT_Y, SCR_W, SCR_H - SOFT_Y, COL_DKGRAY);
    const char *lb[] = { f1, f2, f3, f4, f5, f6 };
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            gfx_SetColor(COL_GRAY);
            gfx_VertLine_NoClip(i*53, SOFT_Y, SCR_H - SOFT_Y);
        }
        txt(i*53 + 2, SOFT_Y + 4, 255, COL_DKGRAY, lb[i]);
    }
}

/* ════════════════════ MENU ════════════════════════════════════ */
static void draw_menu(void) {
    hdr("MAT267 CAS  v1.0");
    txt(4, 21, COL_GRAY, COL_BG, "Keys 1-7 or arrows+ENTER");
    for (int i = 0; i < OP_COUNT; i++) {
        int y = 36 + i*23;
        char buf[32];
        snprintf(buf, 32, " %d: %s", i+1, OP_NAMES[i]);
        if (i == G.menu_cur) {
            fill(0, y-2, SCR_W, 20, COL_NAVY);
            txt(4, y, 255, COL_NAVY, buf);
        } else {
            txt(4, y, COL_FG, COL_BG, buf);
        }
    }
    softbar("SEL", "", "", "", "", "QUIT");
}

/* ════════════════════ ORIENT ══════════════════════════════════ */
static void draw_orient(void) {
    hdr("FLUX: Orientation");
    txt(4, 28, COL_FG, COL_BG, "Normal direction:");
    int y1 = 60, y2 = 90;
    if (G.orient == 1) {
        fill(0, y1-2, SCR_W, 20, COL_NAVY);
        txt(6, y1, 255, COL_NAVY, "> UPWARD / OUTWARD");
        txt(6, y2, COL_FG, COL_BG, "  DOWNWARD / INWARD");
    } else {
        txt(6, y1, COL_FG, COL_BG, "  UPWARD / OUTWARD");
        fill(0, y2-2, SCR_W, 20, COL_NAVY);
        txt(6, y2, 255, COL_NAVY, "> DOWNWARD / INWARD");
    }
    softbar("UP", "DOWN", "", "", "NEXT", "BACK");
}

/* ════════════════════ COORDS ══════════════════════════════════ */
static void draw_coords(void) {
    hdr("TRIPLE: Coordinates");
    txt(4, 28, COL_FG, COL_BG, "Coordinate system:");
    const char *names[3] = {
        "CARTESIAN  (x,y,z)",
        "CYLINDRICAL(r,th,z)",
        "SPHERICAL(rho,phi,th)"
    };
    for (int i = 0; i < 3; i++) {
        int y = 55 + i*30;
        if (G.coords == i) {
            fill(0, y-2, SCR_W, 22, COL_NAVY);
            txt(6, y, 255, COL_NAVY, names[i]);
        } else {
            txt(6, y, COL_FG, COL_BG, names[i]);
        }
    }
    softbar("CART", "CYL", "SPH", "", "NEXT", "BACK");
}

/* ════════════════════ INPUT ═══════════════════════════════════ */
static void draw_input(void) {
    hdr(OP_NAMES[G.op]);

    /* orientation tag for flux */
    if (G.op == OP_FLUX) {
        txt(220, 5, 255, COL_NAVY,
            G.orient == 1 ? "[UP]" : "[DN]");
    }

    int scroll = 0;
    if (G.field_cur >= VISIBLE)
        scroll = G.field_cur - VISIBLE + 1;

    for (int i = 0; i < VISIBLE && (i + scroll) < G.nf; i++) {
        int fi = i + scroll;
        int y  = BODY_TOP + i*26;
        bool active = (fi == G.field_cur);

        /* label */
        txt(4, y, active ? COL_ACCENT : COL_GRAY, COL_BG, G.f[fi].label);

        /* value box */
        int bx = 116, bw = 196, bh = 18;
        if (active) {
            fill(bx, y-1, bw, bh, COL_NAVY);
            char buf[MAX_EXPR + 2];
            snprintf(buf, sizeof(buf), "%s|", G.f[fi].val);
            txt(bx+3, y, 255, COL_NAVY, buf);
        } else {
            gfx_SetColor(COL_ACCENT);
            gfx_Rectangle(bx, y-1, bw, bh);
            txt(bx+3, y, COL_FG, COL_BG, G.f[fi].val);
        }
    }

    /* field counter */
    if (G.nf > VISIBLE) {
        char si[10];
        snprintf(si, 10, "%d/%d", G.field_cur+1, G.nf);
        txt(290, 5, 255, COL_NAVY, si);
    }

    softbar("DEL", "CLR", "PI", "NEG", "NEXT", "CALC");
}

/* ════════════════════ RESULT ══════════════════════════════════ */
static void draw_result(void) {
    hdr(OP_NAMES[G.op]);
    if (G.err[0]) {
        txt(4, 28, COL_RED, COL_BG, "ERROR:");
        txt(4, 44, COL_RED, COL_BG, G.err);
    } else {
        int y = 24;
        for (int i = 0; i < G.nres; i++) {
            txt(4, y, COL_ACCENT, COL_BG, G.res[i].label);
            y += 13;
            txt(12, y, COL_FG, COL_BG, G.res[i].value);
            y += 16;
        }
        gfx_SetColor(COL_GRAY);
        gfx_HorizLine_NoClip(0, y+2, SCR_W);
        char raw[32];
        snprintf(raw, 32, "raw=%.7g", G.raw);
        txt(4, y+6, COL_GRAY, COL_BG, raw);
    }
    softbar("", "", "", "", "REDO", "MENU");
}

/* ════════════════════ DRAW DISPATCH ═══════════════════════════ */
void ui_draw(void) {
    switch (G.screen) {
    case SCR_MENU:   draw_menu();   break;
    case SCR_INPUT:  draw_input();  break;
    case SCR_RESULT: draw_result(); break;
    case SCR_ORIENT: draw_orient(); break;
    case SCR_COORDS: draw_coords(); break;
    }
}

/* ════════════════════ KEY HANDLING ════════════════════════════
   TI-84 Plus CE key rows (kb_Data[1..7]):
     row 1: Graph Trace Zoom Window Yeq 2nd Mode Del
     row 2: (store) . , ( ) / *  (these share row with alpha/vars)
     row 3: Math Apps Prgm Vars Clear
     row 4: x^-1 Sin Cos Tan ^ sqrt x^2 , ( )
     row 5: 7 8 9 * /
     row 6: 4 5 6 + -
     row 7: 1 2 3 0 . (-) Enter
   We check each row's bitmask directly.
   Soft keys Y=, Window, Zoom, Trace, Graph map to F1-F5.
   ════════════════════════════════════════════════════════════ */

static void field_append(char c) {
    Field *f = &G.f[G.field_cur];
    int len  = (int)strlen(f->val);
    if (len < MAX_EXPR - 1) { f->val[len] = c; f->val[len+1] = 0; }
}
static void field_append_str(const char *s) {
    Field *f = &G.f[G.field_cur];
    int len = (int)strlen(f->val);
    strncat(f->val, s, MAX_EXPR - len - 1);
}
static void field_backspace(void) {
    Field *f = &G.f[G.field_cur];
    int len = (int)strlen(f->val);
    if (len > 0) f->val[len-1] = 0;
}
static void field_clear(void) { G.f[G.field_cur].val[0] = 0; }
static void field_negate(void) {
    Field *f = &G.f[G.field_cur];
    if (f->val[0] == '-') {
        memmove(f->val, f->val+1, strlen(f->val));
    } else {
        int len = (int)strlen(f->val);
        if (len < MAX_EXPR-1) { memmove(f->val+1, f->val, len+1); f->val[0] = '-'; }
    }
}
static void next_field(void) {
    if (G.field_cur < G.nf-1) G.field_cur++;
    else { compute(); G.screen = SCR_RESULT; }
}
static void enter_op(Op op) {
    G.op = op; G.nres = 0; G.err[0] = 0;
    if      (op == OP_FLUX)   { G.screen = SCR_ORIENT; return; }
    else if (op == OP_TRIPLE) { G.screen = SCR_COORDS; return; }
    fields_load(op, G.coords);
    fields_defaults();
    G.field_cur = 0;
    G.screen = SCR_INPUT;
}

void ui_key(kb_lkey_t r1, kb_lkey_t r2, kb_lkey_t r3,
            kb_lkey_t r4, kb_lkey_t r5, kb_lkey_t r6,
            kb_lkey_t r7) {

    /* ── MENU ── */
    if (G.screen == SCR_MENU) {
        if (r7 & kb_1) { enter_op(OP_CURLDIV);   return; }
        if (r7 & kb_2) { enter_op(OP_SURFAREA);  return; }
        if (r7 & kb_3) { enter_op(OP_SURFINT);   return; }
        if (r5 & kb_4) { enter_op(OP_FLUX);      return; }
        if (r5 & kb_5) { enter_op(OP_VOLUME);    return; }
        if (r5 & kb_6) { enter_op(OP_TRIPLE);    return; }
        if (r7 & kb_0) { enter_op(OP_TANGPLANE); return; }  /* 0 for 7th */
        if (r1 & kb_Up)   { if (G.menu_cur > 0) G.menu_cur--; return; }
        if (r1 & kb_Down) { if (G.menu_cur < OP_COUNT-1) G.menu_cur++; return; }
        if (r7 & kb_Enter){ enter_op((Op)G.menu_cur); return; }
        if (r3 & kb_Clear){ /* ignore on menu */ return; }
        return;
    }

    /* ── ORIENT ── */
    if (G.screen == SCR_ORIENT) {
        if ((r1 & kb_Yequ)    || (r1 & kb_Up))   G.orient =  1;
        if ((r1 & kb_Window) || (r1 & kb_Down))  G.orient = -1;
        if ((r1 & kb_Graph)  || (r7 & kb_Enter)) {
            fields_load(G.op, G.coords);
            fields_defaults();
            G.field_cur = 0;
            G.screen = SCR_INPUT;
        }
        if (r3 & kb_Clear) G.screen = SCR_MENU;
        return;
    }

    /* ── COORDS ── */
    if (G.screen == SCR_COORDS) {
        if (r1 & kb_Up)      { if (G.coords > 0) G.coords--; }
        if (r1 & kb_Down)    { if (G.coords < 2) G.coords++; }
        if (r1 & kb_Yeq)     G.coords = COORD_CART;
        if (r1 & kb_Window)  G.coords = COORD_CYL;
        if (r1 & kb_Zoom)    G.coords = COORD_SPH;
        if ((r1 & kb_Graph) || (r7 & kb_Enter)) {
            fields_load(G.op, G.coords);
            fields_defaults();
            G.field_cur = 0;
            G.screen = SCR_INPUT;
        }
        if (r3 & kb_Clear) G.screen = SCR_MENU;
        return;
    }

    /* ── RESULT ── */
    if (G.screen == SCR_RESULT) {
        if (r3 & kb_Clear)    G.screen = SCR_INPUT;
        if (r1 & kb_Graph)    { G.screen = SCR_MENU; G.menu_cur = (int)G.op; }
        if (r7 & kb_Enter)    G.screen = SCR_INPUT;
        return;
    }

    /* ── INPUT ── */
    if (G.screen == SCR_INPUT) {
        /* navigation */
        if (r1 & kb_Up)      { if (G.field_cur > 0) G.field_cur--; return; }
        if (r1 & kb_Down)    { next_field(); return; }
        if (r7 & kb_Enter)   { next_field(); return; }
        if (r3 & kb_Clear)   { G.screen = SCR_MENU; return; }

        /* soft keys: Y= Window Zoom Trace Graph */
        if (r1 & kb_Yeq)    { field_backspace(); return; }   /* DEL */
        if (r1 & kb_Window) { field_clear();     return; }   /* CLR */
        if (r1 & kb_Zoom)   { field_append_str("pi"); return; } /* PI */
        if (r1 & kb_Trace)  { field_negate();   return; }    /* NEG */
        if (r1 & kb_Graph)  { compute(); G.screen = SCR_RESULT; return; } /* CALC */

        /* digits */
        if (r7 & kb_1) { field_append('1'); return; }
        if (r7 & kb_2) { field_append('2'); return; }
        if (r7 & kb_3) { field_append('3'); return; }
        if (r5 & kb_4) { field_append('4'); return; }
        if (r5 & kb_5) { field_append('5'); return; }
        if (r5 & kb_6) { field_append('6'); return; }
        if (r5 & kb_7) { field_append('7'); return; }
        if (r5 & kb_8) { field_append('8'); return; }
        if (r5 & kb_9) { field_append('9'); return; }
        if (r7 & kb_0) { field_append('0'); return; }
        if (r7 & kb_DecPnt) { field_append('.'); return; }

        /* operators */
        if (r6 & kb_Add)    { field_append('+'); return; }
        if (r6 & kb_Sub)    { field_append('-'); return; }
        if (r5 & kb_Mul)    { field_append('*'); return; }
        if (r5 & kb_Div)    { field_append('/'); return; }
        if (r4 & kb_Power)  { field_append('^'); return; }
        if (r4 & kb_LParen) { field_append('('); return; }
        if (r4 & kb_RParen) { field_append(')'); return; }

        /* x^2 shortcut */
        if (r4 & kb_Square) { field_append_str("^2"); return; }

        /* trig functions */
        if (r4 & kb_Sin)  { field_append_str("sin(");  return; }
        if (r4 & kb_Cos)  { field_append_str("cos(");  return; }
        if (r4 & kb_Tan)  { field_append_str("tan(");  return; }
        if (r4 & kb_Math) { field_append_str("sqrt("); return; }

        /* variable keys */
        if (r2 & kb_Ln)   { field_append('x'); return; }  /* X,T,θ,n */
        if (r2 & kb_Sto){ field_append('y'); return; }  /* STO→ as y proxy */
    }
}
