#include "cas.h"

State G;

/* 8 custom palette entries (indices 0-7).
   Index 255 = white is always available in the CE palette. */
static const uint16_t PALETTE[8] = {
    gfx_RGBTo1555(0,   0,   0),    /* 0  black   COL_FG      */
    gfx_RGBTo1555(0,   25,  70),   /* 1  navy    COL_NAVY    */
    gfx_RGBTo1555(30,  80,  180),  /* 2  blue    COL_BLUE    */
    gfx_RGBTo1555(45,  45,  45),   /* 3  dkgray  COL_DKGRAY  */
    gfx_RGBTo1555(0,   90,  170),  /* 4  accent  COL_ACCENT  */
    gfx_RGBTo1555(190, 0,   0),    /* 5  red     COL_RED     */
    gfx_RGBTo1555(130, 130, 130),  /* 6  gray    COL_GRAY    */
    gfx_RGBTo1555(255, 255, 255),  /* 7  spare white         */
};

int main(void) {
    /* init graphics */
    gfx_Begin();
    gfx_SetPalette(PALETTE, sizeof(PALETTE), 0);
    gfx_SetDrawBuffer();
    gfx_SetTextScale(1, 1);

    /* init state */
    memset(&G, 0, sizeof(G));
    G.screen   = SCR_MENU;
    G.orient   = 1;
    G.coords   = COORD_SPH;

    while (true) {
        /* draw */
        gfx_FillScreen(COL_BG);
        ui_draw();
        gfx_SwapDraw();

        /* wait for any key */
        kb_Scan();
        while (!kb_AnyKey()) kb_Scan();

        /* read all rows */
        kb_lkey_t r1 = kb_Data[1];
        kb_lkey_t r2 = kb_Data[2];
        kb_lkey_t r3 = kb_Data[3];
        kb_lkey_t r4 = kb_Data[4];
        kb_lkey_t r5 = kb_Data[5];
        kb_lkey_t r6 = kb_Data[6];
        kb_lkey_t r7 = kb_Data[7];

        /* quit: 2nd + Mode */
        if ((r1 & kb_2nd) && (r1 & kb_Mode)) break;

        ui_key(r1, r2, r3, r4, r5, r6, r7);

        /* debounce */
        while (kb_AnyKey()) kb_Scan();
    }

    gfx_End();
    return 0;
}
