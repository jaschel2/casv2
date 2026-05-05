#include "cas.h"

static void sf(int i, const char *lbl, const char *def) {
    strncpy(G.f[i].label, lbl, 13); G.f[i].label[13] = 0;
    strncpy(G.f[i].def,   def,  MAX_EXPR-1); G.f[i].def[MAX_EXPR-1] = 0;
}

void fields_load(Op op, Coords coords) {
    memset(G.f, 0, sizeof(G.f));
    G.nf = 0;

    switch (op) {
    case OP_CURLDIV:
        sf(0,"P(x,y,z)",  "x^2+2*y*z");
        sf(1,"Q(x,y,z)",  "y^2-z^3");
        sf(2,"R(x,y,z)",  "4*x*y*z");
        sf(3,"x0",        "-1");
        sf(4,"y0",        "4");
        sf(5,"z0",        "2");
        G.nf = 6; break;

    case OP_SURFAREA:
        sf(0,"x(u,v)",    "u+2*v");
        sf(1,"y(u,v)",    "6*v");
        sf(2,"z(u,v)",    "u-v");
        sf(3,"u min",     "0");
        sf(4,"u max",     "1");
        sf(5,"v min",     "0");
        sf(6,"v max",     "3");
        G.nf = 7; break;

    case OP_SURFINT:
        sf(0,"x(u,v)",    "u*cos(v)");
        sf(1,"y(u,v)",    "u*sin(v)");
        sf(2,"z(u,v)",    "v");
        sf(3,"f(x,y,z)",  "3*x");
        sf(4,"u min",     "0");
        sf(5,"u max",     "1");
        sf(6,"v min",     "0");
        sf(7,"v max",     "pi/2");
        G.nf = 8; break;

    case OP_FLUX:
        sf(0,"x(u,v)",    "u");
        sf(1,"y(u,v)",    "v");
        sf(2,"z(u,v)",    "v*u");
        sf(3,"P(x,y,z)",  "-y");
        sf(4,"Q(x,y,z)",  "x");
        sf(5,"R(x,y,z)",  "z^2");
        sf(6,"u min",     "0");
        sf(7,"u max",     "2");
        sf(8,"v min",     "0");
        sf(9,"v max",     "1");
        G.nf = 10; break;

    case OP_VOLUME:
        sf(0,"z top(r)",  "4-r^2");
        sf(1,"z bot(r)",  "3*r^2");
        sf(2,"r min",     "0");
        sf(3,"r max",     "1");
        sf(4,"theta min", "0");
        sf(5,"theta max", "2*pi");
        G.nf = 6; break;

    case OP_TRIPLE:
        sf(0,"f(x,y,z)",  "x*z");
        switch (coords) {
        case COORD_CART:
            sf(1,"x min","0"); sf(2,"x max","2");
            sf(3,"y min","0"); sf(4,"y max","2");
            sf(5,"z min","0"); sf(6,"z max","2");
            break;
        case COORD_CYL:
            sf(1,"r min","0");  sf(2,"r max","2");
            sf(3,"th min","0"); sf(4,"th max","2*pi");
            sf(5,"z min","0");  sf(6,"z max","2");
            break;
        case COORD_SPH:
        default:
            sf(1,"rho min","0"); sf(2,"rho max","2");
            sf(3,"phi min","0"); sf(4,"phi max","pi/2");
            sf(5,"th min","0");  sf(6,"th max","pi/2");
            break;
        }
        G.nf = 7; break;

    case OP_TANGPLANE:
        sf(0,"f(x,y)",    "x^2*y+y^2/x");
        sf(1,"x0",        "1");
        sf(2,"y0",        "3");
        G.nf = 3; break;

    default: break;
    }
}

void fields_defaults(void) {
    for (int i = 0; i < G.nf; i++)
        strncpy(G.f[i].val, G.f[i].def, MAX_EXPR-1);
}
