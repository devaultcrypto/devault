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
 * Implementation of the Boneh-Franklin indentity-based encryption.
 *
 * @ingroup cp
 */

#include "relic.h"
#include "relic_test.h"
#include "relic_bench.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int cp_ibe_gen(bn_t master, g1_t pub) {
	bn_t n;
	int result = RLC_OK;

	bn_null(n);

	TRY {
		bn_new(n);

		g1_get_ord(n);
		bn_rand_mod(master, n);

		/* K_pub = sG. */
		g1_mul_gen(pub, master);
	}
	CATCH_ANY {
		result = RLC_ERR;
	}
	FINALLY {
		bn_free(n);
	}
	return result;
}

int cp_ibe_gen_prv(g2_t prv, char *id, int len, bn_t master) {
	g2_map(prv, (uint8_t *)id, len, 1);
	g2_mul(prv, prv, master);
	return RLC_OK;
}

int cp_ibe_enc(uint8_t *out, int *out_len, uint8_t *in, int in_len,
		char *id, int len, g1_t pub) {
	int l, result = RLC_OK;
	uint8_t *buf = NULL, h[RLC_MD_LEN];
	bn_t n;
	bn_t r;
	g1_t p;
	g2_t q;
	gt_t e;

	bn_null(n);
	bn_null(r);
	g1_null(p);
	g2_null(q);
	gt_null(e);

	if (pub == NULL || in_len <= 0 || in_len > RLC_MD_LEN ) {
		return RLC_ERR;
	}

	l = 1 + 2 * RLC_FP_BYTES;
	if (*out_len < (in_len + l)) {
		return RLC_ERR;
	}

	TRY {
		bn_new(n);
		bn_new(r);
		g1_new(p);
		g2_new(q);
		gt_new(e);

		/* Allocate size for storing the output. */
		l = gt_size_bin(e, 0);
		buf = RLC_ALLOCA(uint8_t, l);
		if (buf == NULL) {
			THROW(ERR_NO_MEMORY);
		}

		g1_get_ord(n);

		/* q = H_1(ID). */
		g2_map(q, (uint8_t *)id, len, 1);

		/* e = e(K_pub, q). */
		pc_map(e, pub, q);

		/* h = H_2(e^r). */
		bn_rand_mod(r, n);
		gt_exp(e, e, r);
		gt_write_bin(buf, l, e, 0);
		md_map(h, buf, l);

		/* P = kG. */
		g1_mul_gen(p, r);
		g1_write_bin(out, *out_len, p, 0);

		for (l = 0; l < RLC_MIN(in_len, RLC_MD_LEN); l++) {
			out[l + (2 * RLC_FP_BYTES + 1)] = in[l] ^ h[l];
		}

		*out_len = in_len + (2 * RLC_FP_BYTES + 1);
	} CATCH_ANY {
		result = RLC_ERR;
	} FINALLY {
		bn_free(n);
		bn_free(r);
		g1_free(p);
		g2_free(q);
		gt_free(e);
		RLC_FREE(buf);
	}
	return result;
}

int cp_ibe_dec(uint8_t *out, int *out_len, uint8_t *in, int in_len, g2_t prv) {
	int l, result = RLC_OK;
	uint8_t *buf = NULL, h[RLC_MD_LEN];
	g1_t p;
	gt_t e;

	g1_null(p);
	gt_null(e);

	l = 2 * RLC_FP_BYTES + 1;
	if (prv == NULL || in_len <= l || in_len > (RLC_MD_LEN + l)) {
		return RLC_ERR;
	}

	if (*out_len < (in_len - l)) {
		return RLC_ERR;
	}

	TRY {
		g1_new(p);
		gt_new(e);

		g1_read_bin(p, in, l);

		pc_map(e, p, prv);

		/* Allocate size for storing the output. */
		l = gt_size_bin(e, 0);
		buf = RLC_ALLOCA(uint8_t, l);
		if (buf == NULL) {
			THROW(ERR_NO_MEMORY);
		}
		gt_write_bin(buf, l, e, 0);
		md_map(h, buf, l);

		in_len -= (2 * RLC_FP_BYTES + 1);
		for (l = 0; l < RLC_MIN(in_len, RLC_MD_LEN); l++) {
			out[l] = in[l + (2 * RLC_FP_BYTES + 1)] ^ h[l];
		}

		*out_len = in_len;
	} CATCH_ANY {
		result = RLC_ERR;
	} FINALLY {
		g1_free(p);
		gt_free(e);
		RLC_FREE(buf);
	}

	return result;
}
