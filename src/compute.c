#include "cas.h"

/* ── Simpson 2-D ── */
typedef double (*F2)(double, double, void *);

static double simp2(F2 f, void *ctx,
                    double a1, double b1,
                    double a2, double b2, int n) {
    double hu = (b1-a1)/n, hv = (b2-a2)/n, S = 0.0;
    for (int i = 0; i <= n; i++) {
        double u  = a1 + i*hu;
        double wi = (i==0||i==n) ? 1 : (i%2==0 ? 2 : 4);
        for (int j = 0; j <= n; j++) {
            double v  = a2 + j*hv;
            double wj = (j==0||j==n) ? 1 : (j%2==0 ? 2 : 4);
            S += wi * wj * f(u, v, ctx);
        }
    }
    return S * (hu*hv/9.0);
}

/* ── Simpson 3-D ── */
typedef double (*F3)(double, double, double, void *);

static double simp3(F3 f, void *ctx,
                    double a1, double b1,
                    double a2, double b2,
                    double a3, double b3, int n) {
    double h1=(b1-a1)/n, h2=(b2-a2)/n, h3=(b3-a3)/n, S=0.0;
    for (int i = 0; i <= n; i++) {
        double a  = a1 + i*h1;
        double wi = (i==0||i==n) ? 1 : (i%2==0 ? 2 : 4);
        for (int j = 0; j <= n; j++) {
            double b  = a2 + j*h2;
            double wj = (j==0||j==n) ? 1 : (j%2==0 ? 2 : 4);
            for (int k = 0; k <= n; k++) {
                double c  = a3 + k*h3;
                double wk = (k==0||k==n) ? 1 : (k%2==0 ? 2 : 4);
                S += wi * wj * wk * f(a, b, c, ctx);
            }
        }
    }
    return S * (h1*h2*h3/27.0);
}

/* ── surface helpers ── */
typedef struct { const char *xs, *ys, *zs; } Surf;

static void surf_pt(Surf *s, double u, double v,
                    double *px, double *py, double *pz) {
    *px = cas_eval(s->xs, u, v, 0);
    *py = cas_eval(s->ys, u, v, 0);
    *pz = cas_eval(s->zs, u, v, 0);
}

static void surf_par(Surf *s, double u, double v,
                     double *rux, double *ruy, double *ruz,
                     double *rvx, double *rvy, double *rvz) {
    *rux = nd_u(s->xs, u, v);
    *ruy = nd_u(s->ys, u, v);
    *ruz = nd_u(s->zs, u, v);
    *rvx = nd_v(s->xs, u, v);
    *rvy = nd_v(s->ys, u, v);
    *rvz = nd_v(s->zs, u, v);
}

/* ══ 1. CURL & DIVERGENCE ══════════════════════════════════════ */
static void do_curldiv(void) {
    const char *P = G.f[0].val, *Q = G.f[1].val, *R = G.f[2].val;
    double x0 = cas_eval_lim(G.f[3].val);
    double y0 = cas_eval_lim(G.f[4].val);
    double z0 = cas_eval_lim(G.f[5].val);

    double ci = nd_y(R,x0,y0,z0) - nd_z(Q,x0,y0,z0);
    double cj = nd_z(P,x0,y0,z0) - nd_x(R,x0,y0,z0);
    double ck = nd_x(Q,x0,y0,z0) - nd_y(P,x0,y0,z0);
    double dv = nd_x(P,x0,y0,z0) + nd_y(Q,x0,y0,z0) + nd_z(R,x0,y0,z0);

    char bi[14], bj[14], bk[14], bd[14], bm[14];
    fmt_double(bi,14,ci); fmt_double(bj,14,cj); fmt_double(bk,14,ck);
    fmt_double(bd,14,dv);
    fmt_double(bm,14,sqrt(ci*ci+cj*cj+ck*ck));

    char curl[52];
    snprintf(curl, 52, "<%s,%s,%s>", bi, bj, bk);
    add_res("curl F =", curl);
    add_res("div F =",  bd);
    add_res("|curl| =", bm);
    G.raw = dv;
}

/* ══ 2. SURFACE AREA ═══════════════════════════════════════════ */
static double sa_f(double u, double v, void *p) {
    Surf *s = (Surf *)p;
    double rux,ruy,ruz,rvx,rvy,rvz;
    surf_par(s,u,v,&rux,&ruy,&ruz,&rvx,&rvy,&rvz);
    return cross_mag(rux,ruy,ruz,rvx,rvy,rvz);
}
static void do_surfarea(void) {
    Surf ctx = { G.f[0].val, G.f[1].val, G.f[2].val };
    double r = simp2(sa_f, &ctx,
                     cas_eval_lim(G.f[3].val), cas_eval_lim(G.f[4].val),
                     cas_eval_lim(G.f[5].val), cas_eval_lim(G.f[6].val),
                     SIMP_N);
    char b[20], pi_b[20];
    fmt_double(b, 20, r);
    fmt_double(pi_b, 20, r / M_PI);
    char tmp[28]; snprintf(tmp, 28, "%s*pi", pi_b);
    add_res("SA =", b);
    add_res("  =", tmp);
    G.raw = r;
}

/* ══ 3. SURFACE INTEGRAL ════════════════════════════════════════ */
typedef struct { Surf s; const char *fs; } SICtx;
static double si_f(double u, double v, void *p) {
    SICtx *c = (SICtx *)p;
    double rux,ruy,ruz,rvx,rvy,rvz,px,py,pz;
    surf_par(&c->s,u,v,&rux,&ruy,&ruz,&rvx,&rvy,&rvz);
    surf_pt(&c->s,u,v,&px,&py,&pz);
    return cas_eval(c->fs,px,py,pz) * cross_mag(rux,ruy,ruz,rvx,rvy,rvz);
}
static void do_surfint(void) {
    SICtx ctx;
    ctx.s.xs = G.f[0].val; ctx.s.ys = G.f[1].val; ctx.s.zs = G.f[2].val;
    ctx.fs   = G.f[3].val;
    double r = simp2(si_f, &ctx,
                     cas_eval_lim(G.f[4].val), cas_eval_lim(G.f[5].val),
                     cas_eval_lim(G.f[6].val), cas_eval_lim(G.f[7].val),
                     SIMP_N);
    char b[20], pi_b[20];
    fmt_double(b,20,r); fmt_double(pi_b,20,r/M_PI);
    char tmp[28]; snprintf(tmp,28,"%s*pi",pi_b);
    add_res("Integral =", b);
    add_res("       =", tmp);
    G.raw = r;
}

/* ══ 4. FLUX ════════════════════════════════════════════════════ */
typedef struct { Surf s; const char *Ps,*Qs,*Rs; int ori; } FLCtx;
static double fl_f(double u, double v, void *p) {
    FLCtx *c = (FLCtx *)p;
    double rux,ruy,ruz,rvx,rvy,rvz,px,py,pz,cx,cy,cz;
    surf_par(&c->s,u,v,&rux,&ruy,&ruz,&rvx,&rvy,&rvz);
    surf_pt(&c->s,u,v,&px,&py,&pz);
    cross3(rux,ruy,ruz,rvx,rvy,rvz,&cx,&cy,&cz);
    double dot = cas_eval(c->Ps,px,py,pz)*cx
               + cas_eval(c->Qs,px,py,pz)*cy
               + cas_eval(c->Rs,px,py,pz)*cz;
    return dot * c->ori;
}
static void do_flux(void) {
    FLCtx ctx;
    ctx.s.xs=G.f[0].val; ctx.s.ys=G.f[1].val; ctx.s.zs=G.f[2].val;
    ctx.Ps=G.f[3].val; ctx.Qs=G.f[4].val; ctx.Rs=G.f[5].val;
    ctx.ori=G.orient;
    double r = simp2(fl_f, &ctx,
                     cas_eval_lim(G.f[6].val), cas_eval_lim(G.f[7].val),
                     cas_eval_lim(G.f[8].val), cas_eval_lim(G.f[9].val),
                     SIMP_N);
    char b[20], pi_b[20];
    fmt_double(b,20,r); fmt_double(pi_b,20,r/M_PI);
    char tmp[28]; snprintf(tmp,28,"%s*pi",pi_b);
    add_res("Flux =", b);
    add_res("    =", tmp);
    add_res("Orient:", G.orient==1 ? "up/out" : "down/in");
    G.raw = r;
}

/* ══ 5. VOLUME ══════════════════════════════════════════════════ */
typedef struct { const char *zt, *zb; } VOLCtx;
static double vol_f(double r, double th, void *p) {
    VOLCtx *c = (VOLCtx *)p;
    double top = cas_eval(c->zt, r, th, 0);
    double bot = cas_eval(c->zb, r, th, 0);
    double h   = top - bot;
    return (h > 0 ? h : 0) * r;
}
static void do_volume(void) {
    VOLCtx ctx = { G.f[0].val, G.f[1].val };
    double r = simp2(vol_f, &ctx,
                     cas_eval_lim(G.f[2].val), cas_eval_lim(G.f[3].val),
                     cas_eval_lim(G.f[4].val), cas_eval_lim(G.f[5].val),
                     SIMP_N);
    char b[20], pi_b[20];
    fmt_double(b,20,r); fmt_double(pi_b,20,r/M_PI);
    char tmp[28]; snprintf(tmp,28,"%s*pi",pi_b);
    add_res("Volume =", b);
    add_res("      =", tmp);
    G.raw = r;
}

/* ══ 6. TRIPLE INTEGRAL ═════════════════════════════════════════ */
typedef struct { const char *fs; Coords coords; } TICtx;
static double ti_f(double a, double b, double c_, void *p) {
    TICtx *c = (TICtx *)p;
    double x, y, z, jac;
    switch (c->coords) {
    case COORD_CART:
        x=a; y=b; z=c_; jac=1.0; break;
    case COORD_CYL:
        x=a*cos(b); y=a*sin(b); z=c_; jac=a; break;
    case COORD_SPH:
    default:
        x=a*sin(b)*cos(c_); y=a*sin(b)*sin(c_); z=a*cos(b);
        jac=a*a*sin(b); break;
    }
    return cas_eval(c->fs, x, y, z) * jac;
}
static void do_triple(void) {
    TICtx ctx = { G.f[0].val, G.coords };
    double r = simp3(ti_f, &ctx,
                     cas_eval_lim(G.f[1].val), cas_eval_lim(G.f[2].val),
                     cas_eval_lim(G.f[3].val), cas_eval_lim(G.f[4].val),
                     cas_eval_lim(G.f[5].val), cas_eval_lim(G.f[6].val),
                     SIMP_N3);
    char b[20], pi_b[20];
    fmt_double(b,20,r); fmt_double(pi_b,20,r/M_PI);
    char tmp[28]; snprintf(tmp,28,"%s*pi",pi_b);
    add_res("Integral =", b);
    add_res("       =", tmp);
    const char *cn[] = {"Cartesian","Cylindrical","Spherical"};
    add_res("Coords:", cn[G.coords]);
    G.raw = r;
}

/* ══ 7. TANGENT PLANE ═══════════════════════════════════════════ */
static void do_tangplane(void) {
    const char *f = G.f[0].val;
    double x0 = cas_eval_lim(G.f[1].val);
    double y0 = cas_eval_lim(G.f[2].val);
    double f0 = cas_eval(f, x0, y0, 0);
    double fx  = nd_x(f, x0, y0, 0);
    double fy  = nd_y(f, x0, y0, 0);
    double co  = f0 - fx*x0 - fy*y0;
    char bfx[14], bfy[14], bc[14], bf0[14];
    fmt_double(bfx,14,fx); fmt_double(bfy,14,fy);
    fmt_double(bc,14,co);  fmt_double(bf0,14,f0);
    char line[52];
    snprintf(line, 52, "z=%sx+%sy+%s", bfx, bfy, bc);
    add_res("Tangent plane:", line);
    add_res("f(x0,y0) =", bf0);
    add_res("fx =", bfx);
    add_res("fy =", bfy);
    G.raw = f0;
}

/* ══ DISPATCHER ═════════════════════════════════════════════════ */
void compute(void) {
    G.nres = 0; G.err[0] = '\0'; G.raw = 0.0;
    switch (G.op) {
    case OP_CURLDIV:   do_curldiv();   break;
    case OP_SURFAREA:  do_surfarea();  break;
    case OP_SURFINT:   do_surfint();   break;
    case OP_FLUX:      do_flux();      break;
    case OP_VOLUME:    do_volume();    break;
    case OP_TRIPLE:    do_triple();    break;
    case OP_TANGPLANE: do_tangplane(); break;
    default:
        snprintf(G.err, 72, "Unknown op %d", (int)G.op);
        break;
    }
}
