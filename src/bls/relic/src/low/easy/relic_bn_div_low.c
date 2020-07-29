/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2020 RELIC Authors
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
 * Implementation of the low-level multiple precision integer division
 * functions.
 *
 * @ingroup bn
 */

#include "relic_core.h"
#include "relic_bn.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void bn_divn_low(dig_t *c, dig_t *d, dig_t *a, int sa, dig_t *b, int sb) {
	int norm, i, n, t, sd;
	dig_t carry, t1[3], t2[3];

	/* Normalize x and y so that the leading digit of y is bigger than
	 * 2^(RLC_DIG-1). */
	norm = util_bits_dig(b[sb - 1]) % RLC_DIG;

	if (norm < (int)(RLC_DIG - 1)) {
		norm = (RLC_DIG - 1) - norm;
		carry = bn_lshb_low(a, a, sa, norm);
		if (carry) {
			a[sa++] = carry;
		}
		carry = bn_lshb_low(b, b, sb, norm);
		if (carry) {
			b[sb++] = carry;
		}
	} else {
		norm = 0;
	}

	n = sa - 1;
	t = sb - 1;

	/* Shift y so that the most significant digit of y is aligned with the
	 * most significant digit of x. */
	bn_lshd_low(b, b, sb, (n - t));

	/* Find the most significant digit of the quotient. */
	while (dv_cmp(a, b, sa) != RLC_LT) {
		c[n - t]++;
		bn_subn_low(a, a, b, sa);
	}
	/* Shift y back. */

	bn_rshd_low(b, b, sb + (n - t), (n - t));

	/* Find the remaining digits. */
	for (i = n; i >= (t + 1); i--) {
		if (i > sa) {
			continue;
		}

		if (a[i] == b[t]) {
			c[i - t - 1] = ((((dbl_t)1) << RLC_DIG) - 1);
		} else {
			dbl_t tmp;
			tmp = ((dbl_t)a[i]) << ((dbl_t)RLC_DIG);
			tmp |= (dbl_t)(a[i - 1]);
			tmp /= (dbl_t)(b[t]);
			c[i - t - 1] = (dig_t)tmp;
		}

		c[i - t - 1]++;
		do {
			c[i - t - 1]--;
			t1[0] = (t - 1 < 0) ? 0 : b[t - 1];
			t1[1] = b[t];

			carry = bn_mul1_low(t1, t1, c[i - t - 1], 2);
			t1[2] = carry;

			t2[0] = (i - 2 < 0) ? 0 : a[i - 2];
			t2[1] = (i - 1 < 0) ? 0 : a[i - 1];
			t2[2] = a[i];
		} while (dv_cmp(t1, t2, 3) == RLC_GT);

		carry = bn_mul1_low(d, b, c[i - t - 1], sb);
		sd = sb;
		if (carry) {
			d[sd++] = carry;
		}

		carry = bn_subn_low(a + (i - t - 1), a + (i - t - 1), d, sd);
		sd += (i - t - 1);
		if (sa - sd > 0) {
			carry = bn_sub1_low(a + sd, a + sd, carry, sa - sd);
		}

		if (carry) {
			sd = sb + (i - t - 1);
			carry = bn_addn_low(a + (i - t - 1), a + (i - t - 1), b, sb);
			carry = bn_add1_low(a + sd, a + sd, carry, sa - sd);
			c[i - t - 1]--;
		}
	}
	/* Remainder should be not be longer than the divisor. */
	bn_rshb_low(d, a, sb, norm);
}

void bn_div1_low(dig_t *c, dig_t *d, const dig_t *a, int size, dig_t b) {
	dbl_t w;
	dig_t r;
	int i;

	w = 0;
	for (i = size - 1; i >= 0; i--) {
		w = (w << ((dbl_t)RLC_DIG)) | ((dbl_t)a[i]);

		if (w >= b) {
			r = (dig_t)(w / b);
			w -= ((dbl_t)r) * ((dbl_t)b);
		} else {
			r = 0;
		}
		c[i] = (dig_t)r;
	}
	*d = (dig_t)w;
}
