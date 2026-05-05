#include "cas.h"
#include <ctype.h>

/* recursive-descent parser
   variables: x y z  (also aliased as u v / r theta / rho phi)
   constants: pi  e  (also p = pi)
   ops: + - * / ^ (right-assoc)  unary -
   functions: sin cos tan asin acos atan sqrt ln log exp abs
*/

typedef struct {
    const char *s;
    int         pos;
    double      vx, vy, vz;
    int         err;
} EC;

static double p_expr(EC *c);
static double p_term(EC *c);
static double p_pow (EC *c);
static double p_unary(EC *c);
static double p_primary(EC *c);

static void skip(EC *c) {
    while (c->s[c->pos] == ' ') c->pos++;
}
static int pk(EC *c) {
    skip(c);
    return (unsigned char)c->s[c->pos];
}
static int eat(EC *c, char ch) {
    skip(c);
    if (c->s[c->pos] == ch) { c->pos++; return 1; }
    return 0;
}

double cas_eval(const char *expr, double x, double y, double z) {
    EC c;
    c.s   = expr; c.pos = 0; c.err = 0;
    c.vx  = x;    c.vy  = y;  c.vz  = z;
    double v = p_expr(&c);
    return c.err ? 0.0 : v;
}

double cas_eval_lim(const char *s) {
    return cas_eval(s, 0, 0, 0);
}

static double p_expr(EC *c) {
    double v = p_term(c);
    for (;;) {
        if      (eat(c, '+')) v += p_term(c);
        else if (eat(c, '-')) v -= p_term(c);
        else break;
    }
    return v;
}

static double p_term(EC *c) {
    double v = p_pow(c);
    for (;;) {
        if      (eat(c, '*')) { v *= p_pow(c); }
        else if (eat(c, '/')) { double d = p_pow(c); v = d ? v/d : 0; }
        else break;
    }
    return v;
}

static double p_pow(EC *c) {
    double v = p_unary(c);
    if (!c->err && eat(c, '^'))
        v = pow(v, p_pow(c));  /* right-associative */
    return v;
}

static double p_unary(EC *c) {
    if (eat(c, '-')) return -p_primary(c);
    eat(c, '+');
    return p_primary(c);
}

static double p_primary(EC *c) {
    skip(c);
    int ch = pk(c);

    /* number literal */
    if (isdigit(ch) || ch == '.') {
        char buf[32]; int i = 0;
        while (i < 31) {
            ch = pk(c);
            if (!isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E') break;
            buf[i++] = (char)c->s[c->pos++];
            if ((ch=='e'||ch=='E') &&
                (c->s[c->pos]=='+'||c->s[c->pos]=='-'))
                buf[i++] = (char)c->s[c->pos++];
        }
        buf[i] = 0;
        return atof(buf);
    }

    /* parentheses */
    if (eat(c, '(')) {
        double v = p_expr(c);
        eat(c, ')');
        return v;
    }

    /* identifier */
    if (isalpha(ch) || ch == '_') {
        char nm[16]; int i = 0;
        while (i < 15 && (isalnum(pk(c)) || pk(c) == '_'))
            nm[i++] = (char)c->s[c->pos++];
        nm[i] = 0;

        /* constants */
        if (!strcmp(nm,"pi") || !strcmp(nm,"p")) return M_PI;
        if (!strcmp(nm,"e"))                      return M_E;

        /* variables */
        if (!strcmp(nm,"x")||!strcmp(nm,"u")||
            !strcmp(nm,"r")||!strcmp(nm,"rho"))         return c->vx;
        if (!strcmp(nm,"y")||!strcmp(nm,"v")||
            !strcmp(nm,"theta")||!strcmp(nm,"phi"))      return c->vy;
        if (!strcmp(nm,"z"))                             return c->vz;

        /* functions — must be followed by '(' */
        if (eat(c, '(')) {
            double a = p_expr(c);
            double b = 0.0;
            if (eat(c, ',')) b = p_expr(c);
            eat(c, ')');

            if (!strcmp(nm,"sin"))   return sin(a);
            if (!strcmp(nm,"cos"))   return cos(a);
            if (!strcmp(nm,"tan"))   return tan(a);
            if (!strcmp(nm,"asin"))  return asin(a);
            if (!strcmp(nm,"acos"))  return acos(a);
            if (!strcmp(nm,"atan"))  return atan(a);
            if (!strcmp(nm,"atan2")) return atan2(a, b);
            if (!strcmp(nm,"sqrt"))  return sqrt(fabs(a));
            if (!strcmp(nm,"ln"))    return a > 0 ? log(a)   : 0.0;
            if (!strcmp(nm,"log"))   return a > 0 ? log10(a) : 0.0;
            if (!strcmp(nm,"exp"))   return exp(a);
            if (!strcmp(nm,"abs"))   return fabs(a);
            if (!strcmp(nm,"pow"))   return pow(a, b);
        }
        c->err = 1;
        return 0.0;
    }

    c->err = 1;
    return 0.0;
}
