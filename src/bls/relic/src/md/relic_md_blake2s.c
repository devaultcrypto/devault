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
 * Implementation of the BLAKE2s hash function.
 *
 * @ingroup md
 */

#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "relic_util.h"
#include "relic_md.h"
#include "blake2.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if MD_MAP == B2S160 || !defined(STRIP)

void md_map_b2s160(uint8_t *hash, const uint8_t *msg, int len) {
	blake2s(hash, msg, 0, 20, len, 0);
}

#endif

#if MD_MAP == B2S256 || !defined(STRIP)

void md_map_b2s256(uint8_t *hash, const uint8_t *msg, int len) {
	blake2s(hash, msg, 0, 32, len, 0);
}

#endif
