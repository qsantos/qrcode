#include "correction.h"

#include <string.h>

// error correction algorithms are inspired from
// https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders

static byte gf_exp[512] = {0};
static byte gf_log[256] = {0};

static void gf_init()
{
	static char done = 0;
	if (done) return;
	done = 1;
	for (size_t i = 0, x = 1; i < 255; i++, x<<=1)
	{
		if (x & 0x100)
			x ^= 0x11d;
		gf_exp[i] = x;
		gf_log[x] = i;
	}
	for (size_t i = 255; i < 512; i++)
		gf_exp[i] = gf_exp[i-255];
}

static byte gf_mul(byte x, byte y)
{
	if (!x || !y) return 0;
	return gf_exp[gf_log[x] + gf_log[y]];
}

static byte gf_div(byte x, byte y)
{
	if (x == 0) return 0;
	if (y == 0) return 0; // TODO: error
	return gf_exp[gf_log[x] + 255 - gf_log[y]];
}

static void gf_poly_scale(poly_t* r, poly_t* p, byte x)
{
	for (size_t i = 0; i <= p->d; i++)
		r->c[i] = gf_mul(p->c[i], x);
	r->d = p->d;
}

static byte max(byte a, byte b)
{
	return a >= b ? a : b;
}
static void gf_poly_add(poly_t* dest, poly_t* p, poly_t* q)
{
	poly_t r;

	size_t d = max(p->d, q->d);
	for (size_t i = 0; i <= p->d; i++)
		r.c[i+d-p->d] = p->c[i];
	for (size_t i = 0; i <= q->d; i++)
		r.c[i+d-q->d] ^= q->c[i];
	r.d = d;

	memcpy(dest, &r, sizeof(poly_t));
}

static void gf_poly_mul(poly_t* dest, poly_t* p, poly_t* q)
{
	poly_t r = { p->d + q->d, {0} };
	for (size_t i = 0; i <= p->d; i++)
		for (size_t j = 0; j <= q->d; j++)
			r.c[i+j] ^= gf_mul(p->c[i], q->c[j]);
	memcpy(dest, &r, sizeof(poly_t));
}

static byte gf_poly_eval(poly_t* p, byte x)
{
	byte y = p->c[0];
	for (size_t i = 1; i <= p->d; i++)
		y = gf_mul(y, x) ^ p->c[i];
	return y;
}

byte rs_calc_syndromes(poly_t* msg, poly_t* synd)
{
	gf_init();
	byte ret = 0;
	for (size_t i = 0; i < synd->d; i++)
		if ((synd->c[i] = gf_poly_eval(msg, gf_exp[i])) != 0)
			ret = 1;
	return ret;
}

byte rs_find_error(poly_t* msg, poly_t* synd, poly_t* pos)
{
	// find error locator polynomial with Berlekamp-Massey algorithm
	poly_t err = { 0, {1} };
	poly_t old = { 0, {1} };
	for (size_t i = 0; i < synd->d; i++)
	{
		old.d++;
		old.c[old.d] = 0;
		byte delta = synd->c[i];
		for (size_t j = 1; j <= err.d; j++)
			delta ^= gf_mul(err.c[err.d-j], synd->c[i-j]);
		if (delta)
		{
			poly_t new;
			if (old.d > err.d)
			{
				gf_poly_scale(&new, &old, delta);
				gf_poly_scale(&old, &err, gf_div(1,delta));
				memcpy(&err, &new, sizeof(poly_t));
			}
			gf_poly_scale(&new, &old, delta);
			gf_poly_add(&err, &err, &new);
		}
	}
	if (err.d > synd->d/2)
		return 1; // too many errors to correct

	// find zeros of error polynomial
	size_t found = 0;
	for (size_t i = 0; i <= msg->d; i++)
		if (gf_poly_eval(&err, gf_exp[255-i]) == 0)
		{
			pos->c[pos->d++] = msg->d - i;
			found++;
		}

	if (found < err.d)
		return 1; // couldn't find error locations
	
	return 0;
}

static void rs_fderivative(poly_t* r, poly_t* p)
{
	size_t i = p->d&1 ? 0 : 1;
	for (; i < p->d; i+=2)
		r->c[i/2] = p->c[i];
	r->d = p->d/2;
}
void rs_correct_errata(poly_t* msg, poly_t* synd, poly_t* pos)
{
	// calculate error locator polynomial
	poly_t q = { 0, {1} };
	for (size_t i = 0; i < pos->d; i++)
	{
		byte x = gf_exp[msg->d - pos->c[i]];
		poly_t tmp = { 1, {x,1} };
		gf_poly_mul(&q, &q, &tmp);
	}

	// calculate error evaluator polynomial
	// Python equiv. p = synd[0:len(pos)].reverse()
	poly_t p = { pos->d-1, {0} };
	for (size_t i = 0; i <= p.d; i++)
		p.c[i] = synd->c[p.d-i];

	gf_poly_mul(&p, &p, &q);

	// Python equiv. p = p[len(p)-len(pos):len(p)]
	memmove(p.c, p.c + p.d-pos->d+1, pos->d);
	p.d = pos->d-1;

	// formal derivative of error locator eliminates even terms
	poly_t qprime;
	rs_fderivative(&qprime, &q);

	// compute corrections
	for (size_t i = 0; i < pos->d; i++)
	{
		byte x = gf_exp[pos->c[i] + 255 - msg->d];
		byte y = gf_poly_eval(&p, x);
		byte z = gf_poly_eval(&qprime, gf_mul(x,x));
		msg->c[pos->c[i]] ^= gf_div(y, gf_mul(x,z));
	}
}
