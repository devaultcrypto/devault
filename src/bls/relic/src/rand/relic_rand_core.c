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
 * Implementation of utilities for pseudo-random number generation.
 *
 * @ingroup rand
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_label.h"
#include "relic_rand.h"
#include "relic_md.h"
#include "relic_err.h"

#if RAND == UDEV || SEED == DEV || SEED == UDEV

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#elif SEED == WCGR

/* Avoid redefinition warning. */
#undef ERROR
#undef WSIZE
#undef DOUBLE

#include <windows.h>
//#include <Wincrypt.h>

#elif SEED == RDRND

#include <immintrin.h>

#endif

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * The path to the char device that supplies entropy.
 */
#if SEED == DEV
#define RLC_RAND_PATH		"/dev/random"
#else
#define RLC_RAND_PATH		"/dev/urandom"
#endif

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void rand_init(void) {
	uint8_t buf[RLC_RAND_SEED];

#if RAND == UDEV
	int *fd = (int *)&(core_get()->rand);

	*fd = open(RLC_RAND_PATH, O_RDONLY);
	if (*fd == -1) {
		RLC_THROW(ERR_NO_FILE);
	}
#else

#if !defined(SEED)

	memset(buf, 0, RLC_RAND_SEED);

#elif SEED == DEV || SEED == UDEV
	int fd, c, l;

	fd = open(RLC_RAND_PATH, O_RDONLY);
	if (fd == -1) {
		RLC_THROW(ERR_NO_FILE);
	}

	l = 0;
	do {
		c = read(fd, buf + l, RLC_RAND_SEED - l);
		l += c;
		if (c == -1) {
			RLC_THROW(ERR_NO_READ);
		}
	} while (l < RLC_RAND_SEED);

	if (fd != -1) {
		close(fd);
	}
#elif SEED == LIBC

#if OPSYS == FREEBSD
	/* This is better than using a fixed value. */
	srandomdev();
	for (int i = 0; i < RLC_RAND_SEED; i++) {
		buf[i] = (uint8_t)random();
	}
#else
	/* This is horribly insecure, serves only for benchmarking. */
	srand(1);
	for (int i = 0; i < RLC_RAND_SEED; i++) {
		buf[i] = (uint8_t)rand();
	}
#endif

#elif SEED == WCGR

	HCRYPTPROV hCryptProv;

	if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL,
					CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
		RLC_THROW(ERR_NO_FILE);
	}
	if (hCryptProv && !CryptGenRandom(hCryptProv, RLC_RAND_SEED, buf)) {
		RLC_THROW(ERR_NO_READ);
	}
	if (hCryptProv && !CryptReleaseContext(hCryptProv, 0)) {
		RLC_THROW(ERR_NO_READ);
	}

#elif SEED == RDRND

	int i, j;
	ull_t r;

	while (i < RLC_RAND_SEED) {
#ifdef __RDRND__
		while (_rdrand64_step(&r) == 0);
#else
#error "RdRand not available, check your compiler settings."
#endif
		for (j = 0; i < RLC_RAND_SEED && j < sizeof(ull_t); i++, j++) {
			buf[i] = r & 0xFF;
		}
	}

#endif

#endif /* RAND == UDEV */

#if RAND != CALL
	core_get()->seeded = 0;
	rand_seed(buf, RLC_RAND_SEED);
#else
	rand_seed(NULL, NULL);
#endif
}

void rand_clean(void) {
	ctx_t *ctx = core_get();
	if (ctx != NULL) {
#if RAND == UDEV
		int *fd = (int *)&(ctx->rand);
		close(*fd);
#endif
#if RAND != CALL
		memset(ctx->rand, 0, sizeof(ctx->rand));
#else
		ctx->rand_call = NULL;
		ctx->rand_args = NULL;
#endif
		ctx->seeded = 0;
	}
}
