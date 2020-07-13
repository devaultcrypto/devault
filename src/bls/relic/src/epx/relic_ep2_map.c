/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2019 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of hashing to a prime elliptic curve over a quadratic
 * extension.
 *
 * @ingroup epx
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_tmpl_map.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#ifdef EP_CTMAP
/**
 * Evaluate a polynomial represented by its coefficients using Horner's rule.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the input value.
 * @param[in] coeffs		- the vector of coefficients in the polynomial.
 * @param[in] len			- the degree of the polynomial.
 */
TMPL_MAP_HORNER(fp2, fp2_t)

/**
 * Generic isogeny map evaluation for use with SSWU map.
 */
TMPL_MAP_ISOGENY_MAP(2)
#endif /* EP_CTMAP */

/**
 * Simplified SWU mapping.
 */
#define EP2_MAP_COPY_COND(O, I, C)                                                       \
	do {                                                                                 \
		dv_copy_cond(O[0], I[0], RLC_FP_DIGS, C);                                        \
		dv_copy_cond(O[1], I[1], RLC_FP_DIGS, C);                                        \
	} while (0)
TMPL_MAP_SSWU(2,fp_t,EP2_MAP_COPY_COND)

/**
 * Shallue--van de Woestijne map.
 */
TMPL_MAP_SVDW(2,fp_t,EP2_MAP_COPY_COND)
#undef EP2_MAP_COPY_COND

/**
 * Multiplies a point by the cofactor in a Barreto-Naehrig curve.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 */
static void ep2_mul_cof_bn(ep2_t r, ep2_t p) {
	bn_t x;
	ep2_t t0, t1, t2;

	ep2_null(t0);
	ep2_null(t1);
	ep2_null(t2);
	bn_null(x);

	TRY {
		ep2_new(t0);
		ep2_new(t1);
		ep2_new(t2);
		bn_new(x);

		fp_prime_get_par(x);

		/* Compute t0 = xP. */
		ep2_mul_basic(t0, p, x);

		/* Compute t1 = \psi(3xP). */
		ep2_dbl(t1, t0);
		ep2_add(t1, t1, t0);
		ep2_norm(t1, t1);
		ep2_frb(t1, t1, 1);

		/* Compute t2 = \psi^3(P) + t0 + t1 + \psi^2(xP). */
		ep2_frb(t2, p, 2);
		ep2_frb(t2, t2, 1);
		ep2_add(t2, t2, t0);
		ep2_add(t2, t2, t1);
		ep2_frb(t1, t0, 2);
		ep2_add(t2, t2, t1);

		ep2_norm(r, t2);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t0);
		ep2_free(t1);
		ep2_free(t2);
		bn_free(x);
	}
}

/**
 * Multiplies a point by the cofactor in a Barreto-Lynn-Scott curve.
 *
 * @param[out] r			- the result.
 * @param[in] p				- the point to multiply.
 */
static void ep2_mul_cof_b12(ep2_t r, ep2_t p) {
	bn_t x;
	ep2_t t0, t1, t2, t3;

	ep2_null(t0);
	ep2_null(t1);
	ep2_null(t2);
	ep2_null(t3);
	bn_null(x);

	TRY {
		ep2_new(t0);
		ep2_new(t1);
		ep2_new(t2);
		ep2_new(t3);
		bn_new(x);

		fp_prime_get_par(x);

		/* Compute t0 = xP. */
		ep2_mul_basic(t0, p, x);
		/* Compute t1 = [x^2]P. */
		ep2_mul_basic(t1, t0, x);

		/* t2 = (x^2 - x - 1)P = x^2P - x*P - P. */
		ep2_sub(t2, t1, t0);
		ep2_sub(t2, t2, p);
		/* t3 = \psi(x - 1)P. */
		ep2_sub(t3, t0, p);
		ep2_frb(t3, t3, 1);
		ep2_add(t2, t2, t3);
		/* t3 = \psi^2(2P). */
		ep2_dbl(t3, p);
		ep2_frb(t3, t3, 2);
		ep2_add(t2, t2, t3);
		ep2_norm(r, t2);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep2_free(t0);
		ep2_free(t1);
		ep2_free(t2);
		ep2_free(t3);
		bn_free(x);
	}
}

static inline int fp2_sgn0(const fp2_t t, bn_t k, const bn_t pm1o2) {
	const int t_0_zero = fp_is_zero(t[0]);

	fp_prime_back(k, t[0]);
	const int t_0_neg = bn_cmp(k, pm1o2);

	fp_prime_back(k, t[1]);
	const int t_1_neg = bn_cmp(k, pm1o2);

	/* t[0] == 0 ? sgn0(t[1]) : sgn0(t[0]) */
	return (!t_0_zero) * t_0_neg + t_0_zero * t_1_neg;
}

void ep2_map_impl(ep2_t p, const uint8_t *msg, int len, const uint8_t *dst, int dst_len) {
	bn_t k, pm1o2;
	fp2_t t;
	ep2_t q;
	int neg;
	/* enough space for two extension field elements plus extra bytes for uniformity */
	const int len_per_elm = (FP_PRIME + ep_param_level() + 7) / 8;
	uint8_t *pseudo_random_bytes = RLC_ALLOCA(uint8_t, 4 * len_per_elm);

	bn_null(k);
	bn_null(pm1o2);
	fp2_null(t);
	ep2_null(q);

	TRY {
		bn_new(k);
		bn_new(pm1o2);
		fp2_new(t);
		ep2_new(q);

		/* which hash function should we use? */
		const int abNeq0 = (ep2_curve_opt_a() != RLC_ZERO) && (ep2_curve_opt_b() != RLC_ZERO);
		void (*const map_fn)(ep2_t, fp2_t) = (ep2_curve_is_ctmap() || abNeq0) ? ep2_map_sswu : ep2_map_svdw;

		/* XXX(rsw) See note in ep/relic_ep_map.c about sgn0 variants. */
		/* (p-1)/2 for detecting sign of y */
		pm1o2->sign = RLC_POS;
		pm1o2->used = RLC_FP_DIGS;
		dv_copy(pm1o2->dp, fp_prime_get(), RLC_FP_DIGS);
		bn_hlv(pm1o2, pm1o2);

		/* XXX(rsw) See note in ep/relic_ep_map.c about using MD_MAP. */
		/* hash to a pseudorandom string using md_xmd */
		md_xmd(pseudo_random_bytes, 4 * len_per_elm, msg, len, dst, dst_len);

#define EP2_MAP_CONVERT_BYTES(IDX)                                                       \
	do {                                                                                 \
		bn_read_bin(k, pseudo_random_bytes + 2 * IDX * len_per_elm, len_per_elm);        \
		fp_prime_conv(t[0], k);                                                          \
		bn_read_bin(k, pseudo_random_bytes + (2 * IDX + 1) * len_per_elm, len_per_elm);  \
		fp_prime_conv(t[1], k);                                                          \
	} while (0)

#define EP2_MAP_APPLY_MAP(PT)                                                            \
	do {                                                                                 \
		/* sign of t */                                                                  \
		neg = fp2_sgn0(t, k, pm1o2);                                                     \
		/* convert */                                                                    \
		map_fn(PT, t);                                                                   \
		/* compare sign of y to sign of t; fix if necessary */                           \
		neg = neg != fp2_sgn0(PT->y, k, pm1o2);                                          \
		fp2_neg(t, PT->y);                                                               \
		dv_copy_cond(PT->y[0], t[0], RLC_FP_DIGS, neg);                                  \
		dv_copy_cond(PT->y[1], t[1], RLC_FP_DIGS, neg);                                  \
	} while (0)

		/* first map invocation */
		EP2_MAP_CONVERT_BYTES(0);
		EP2_MAP_APPLY_MAP(p);

		/* second map invocation */
		EP2_MAP_CONVERT_BYTES(1);
		EP2_MAP_APPLY_MAP(q);

#undef EP2_MAP_CONVERT_BYTES
#undef EP2_MAP_APPLY_MAP

		/* sum the result */
		ep2_add(p, p, q);
		ep2_norm(p, p);

		/* clear cofactor */
		switch (ep_curve_is_pairf()) {
			case EP_BN:
				ep2_mul_cof_bn(p, p);
				break;
			case EP_B12:
				ep2_mul_cof_b12(p, p);
				break;
			default:
				/* Now, multiply by cofactor to get the correct group. */
				ep2_curve_get_cof(k);
				if (bn_bits(k) < RLC_DIG) {
					ep2_mul_dig(p, p, k->dp[0]);
				} else {
					ep2_mul_basic(p, p, k);
				}
				break;
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(k);
		bn_free(pm1o2);
		fp2_free(t);
		ep2_free(q);
		RLC_FREE(pseudo_random_bytes);
	}
}

/**
 * Based on the rust implementation of pairings, zkcrypto/pairing.
 * The algorithm is Shallue–van de Woestijne encoding from
 * Section 3 of "Indifferentiable Hashing to Barreto–Naehrig Curves"
 * from Fouque-Tibouchi: <https://www.di.ens.fr/~fouque/pub/latincrypt12.pdf>
 */
void ep2_sw_encode(ep2_t p, fp2_t t) {
	if (fp2_is_zero(t)) {
		// Maps t=0 to the point at infinity.
		ep2_set_infty(p);
		return;
	}
	fp2_t nt; // Negative t
	fp2_t w;
	bn_t s_n3;
	bn_t s_n3m1o2;
	fp2_t x1;
	fp2_t x2;
	fp2_t x3;
	fp2_t rhs;

	fp2_new(nt);
	fp2_new(w);
	fp2_new(b);
	bn_new(s_n3);
	bn_new(s_n3m1o2);
	fp2_new(x1);
	fp2_new(x2);
	fp2_new(x3);
	fp2_new(rhs);

	// nt = -t
	fp2_neg(nt, t);

	uint8_t buf0[RLC_FP_BYTES];  // t[1]
	uint8_t buf1[RLC_FP_BYTES];  // -t[1]
	fp_write_bin(buf0, RLC_FP_BYTES, t[1]);
	fp_write_bin(buf1, RLC_FP_BYTES, nt[1]);

	// Compare bytes since fp_cmp compares montgomery representation
	int parity = memcmp(buf0, buf1, RLC_FP_BYTES) > 0;

	// w = t^2 + b + 1
	fp2_mul(w, t, t);
	fp2_add(w, w, ep2_curve_get_b());
	fp_add_dig(w[0], w[0], 1);

	if (fp2_is_zero(w)) {
		ep2_curve_get_gen(p);
		if (parity) {
			ep2_neg(p, p);
		}
		return;
	}

	// sqrt(-3)
	ep2_curve_get_s3(s_n3);
	fp2_t s_n3p;
	fp2_null(s_n3p);
	fp2_new(s_n3p);
	fp2_t s_n3m1o2p;
	fp2_null(s_n3m1o2p);
	fp2_new(s_n3m1o2p);

	fp2_zero(s_n3p);
	fp2_zero(s_n3m1o2p);
	fp_prime_conv(s_n3p[0], s_n3);

	// (sqrt(-3) - 1) / 2
	ep2_curve_get_s32(s_n3m1o2);
	fp_prime_conv(s_n3m1o2p[0], s_n3m1o2);

	fp2_inv(w, w);
	fp2_mul(w, w, s_n3p);
	fp2_mul(w, w, t);

	// x1 = -wt + sqrt(-3)
	fp2_neg(x1, w);
	fp2_mul(x1, x1, t);
	fp2_add(x1, x1, s_n3m1o2p);

	// x2 = -x1 - 1
	fp2_neg(x2, x1);
	fp_sub_dig(x2[0], x2[0], 1);

	// x3 = 1/w^2 + 1
	fp2_mul(x3, w, w);
	fp2_inv(x3, x3);
	fp_add_dig(x3[0], x3[0], 1);

	fp2_zero(p->y);
	fp2_set_dig(p->z, 1);

	// if x1 has no y, try x2. if x2 has no y, try x3
	fp2_copy(p->x, x1);

	ep2_rhs(rhs, p);
	int Xx1 = fp2_srt(p->y, rhs) ? 1 : -1;
	fp2_copy(p->x, x2);
	ep2_rhs(rhs, p);
	int Xx2 = fp2_srt(p->y, rhs) ? 1 : -1;

	// This formula computes which index to use, in constant time
	// without conditional branches. It's taken from the paper. 3 is
	// added, because the % operator in c can be negative.
	int index = ((((Xx1 - 1) * Xx2) % 3) + 3) % 3;

	if (index == 0) {
		fp2_copy(p->x, x1);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	} else if (index == 1) {
		fp2_copy(p->x, x2);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	} else if (index == 2) {
		fp2_copy(p->x, x3);
		ep2_rhs(rhs, p);
		fp2_srt(p->y, rhs);
	}

	p->norm = 1;

	// Check for lexicografically greater than the negation
	fp2_t ny;
	fp2_null(ny);
	fp2_new(ny);
	fp2_neg(ny, p->y);
	fp_write_bin(buf0, RLC_FP_BYTES, p->y[1]);
	fp_write_bin(buf1, RLC_FP_BYTES, ny[1]);

	// Compare bytes since fp_cmp compares montgomery representation
	if ((memcmp(buf0, buf1, RLC_FP_BYTES) > 0) != parity) {
		ep2_neg(p, p);
	}

	fp_free(nt);
	fp2_free(w);
	fp2_free(b);
	bn_free(s_n3);
	bn_free(s_n3m1o2);
	fp2_free(x1);
	fp2_free(x2);
	fp2_free(x3);
	fp2_free(rhs);
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void ep2_map(ep2_t p, const uint8_t *msg, int len) {
	ep2_map_impl(p, msg, len, (const uint8_t *)"RELIC", 5);
}

void ep2_map_ft(ep2_t p, const uint8_t *msg, int len) {
    int performHash = 0;
	TRY {
		uint8_t input[RLC_MD_LEN + 8];
		if (performHash) {
			md_map(input, msg, len);
		} else {
			if (len != RLC_MD_LEN) {
				THROW(ERR_CAUGHT);
			}
			memcpy(input, msg, len);
		}
		// b"G2_0_c0"
		input[RLC_MD_LEN + 0] = 0x47; // G
		input[RLC_MD_LEN + 1] = 0x32; // 2
		input[RLC_MD_LEN + 2] = 0x5f; // _
		input[RLC_MD_LEN + 3] = 0x30; // 0
		input[RLC_MD_LEN + 4] = 0x5f; // _
		input[RLC_MD_LEN + 5] = 0x63; // c
		input[RLC_MD_LEN + 6] = 0x30; // 0

		bn_t t00;
		bn_t t01;
		bn_t t10;
		bn_t t11;
		fp_t t00p;
		fp_t t01p;
		fp_t t10p;
		fp_t t11p;
		fp2_t t0p;
		fp2_t t1p;
		ep2_t p0;
		ep2_t p1;

		bn_new(t00);
		bn_new(t01);
		bn_new(t10);
		bn_new(t11);
		fp_new(t00p);
		fp_new(t01p);
		fp_new(t10p);
		fp_new(t11p);
		fp2_new(t0p);
		fp2_new(t1p);
        ep2_new(p1);
		ep2_new(p0);

		uint8_t t00Bytes[RLC_MD_LEN * 2];
		uint8_t t01Bytes[RLC_MD_LEN * 2];
		uint8_t t10Bytes[RLC_MD_LEN * 2];
		uint8_t t11Bytes[RLC_MD_LEN * 2];

		// b"G2_0_c0"
		input[RLC_MD_LEN + 7] = 0x00; // 0
		md_map(t00Bytes, input, RLC_MD_LEN + 8);
		input[RLC_MD_LEN + 7] = 1;    // 1
		md_map(t00Bytes + RLC_MD_LEN, input, RLC_MD_LEN + 8);

		// b"G2_0_c1"
		input[RLC_MD_LEN + 6] = 0x31; // b"1"
		input[RLC_MD_LEN + 7] = 0;    // 0

		md_map(t01Bytes, input, RLC_MD_LEN + 8);
		input[RLC_MD_LEN + 7] = 1;    // 1
		md_map(t01Bytes + RLC_MD_LEN, input, RLC_MD_LEN + 8);

		// b"G2_1_c0"
		input[RLC_MD_LEN + 3] = 0x31;  // b"1"
		input[RLC_MD_LEN + 6] = 0x30;  // b"0"
		input[RLC_MD_LEN + 7] = 0;     // 0
		md_map(t10Bytes, input, RLC_MD_LEN + 8);
		input[RLC_MD_LEN + 7] = 1;     // 1
		md_map(t10Bytes + RLC_MD_LEN, input, RLC_MD_LEN + 8);

		// b"G2_1_c1"
		input[RLC_MD_LEN + 6] = 0x31;     // b"1"
		input[RLC_MD_LEN + 7] = 0;     // 0
		md_map(t11Bytes, input, RLC_MD_LEN + 8);
		input[RLC_MD_LEN + 7] = 1;     // 1
		md_map(t11Bytes + RLC_MD_LEN, input, RLC_MD_LEN + 8);

		bn_read_bin(t00, t00Bytes, RLC_MD_LEN * 2);
		bn_read_bin(t01, t01Bytes, RLC_MD_LEN * 2);
		bn_read_bin(t10, t10Bytes, RLC_MD_LEN * 2);
		bn_read_bin(t11, t11Bytes, RLC_MD_LEN * 2);

		fp_prime_conv(t00p, t00);
		fp_prime_conv(t01p, t01);
		fp_prime_conv(t10p, t10);
		fp_prime_conv(t11p, t11);

		fp_copy(t0p[0], t00p);
		fp_copy(t0p[1], t01p);
		fp_copy(t1p[0], t10p);
		fp_copy(t1p[1], t11p);

		ep2_sw_encode(p0, t0p);
		ep2_sw_encode(p1, t1p);
		ep2_add(p0, p0, p1);

		/* Now, multiply by cofactor to get the correct group. */
		switch (ep_param_get()) {
			case BN_P158:
			case BN_P254:
			case BN_P256:
			case BN_P382:
			case BN_P638:
				ep2_mul_cof_bn(p, p0);
				break;
			case B12_P381:
			case B12_P455:
			case B12_P638:
				ep2_mul_cof_b12(p, p0);
				break;
			default: {
				bn_t x;
				bn_new(x);
				ep2_curve_get_cof(x);
				if (bn_bits(x) < RLC_DIG) {
					ep2_mul_dig(p, p0, x->dp[0]);
					if (bn_sign(x) == RLC_NEG) {
						ep2_neg(p, p);
					}
				} else {
					ep2_mul(p, p0, x);
				}
				bn_free(k);
				break;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		ep_free(p0);
		ep_free(p1);
		fp2_free(t0p);
		fp2_free(t1p);
		fp_free(t00p);
		fp_free(t01p);
		fp_free(t10p);
		fp_free(t11p);
		bn_free(t00);
		bn_free(t01);
		bn_free(t10);
		bn_free(t11);
		ep2_free(p0);
		ep2_free(p1);
	}
}