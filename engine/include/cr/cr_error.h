/*
 * cr_error.h - error-code family for the recovery stack.
 *
 * Reproduces the reference engine convention (HRESULT-style 0x5002xxxx family,
 * corpus: 05-scan-engine-architecture.md section 4) while extending it with
 * explicit codes our layers need. Success is always 0.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ERROR_H_
#define CR_ERROR_H_

#include "cr_types.h"

namespace cr {

enum Err : u32 {
    OK = 0,

    /* --- open / transport (reference open-fail class 0x5002...) --- */
    ERR_OPEN_FAILED        = 0x50020001,
    ERR_READ_FAILED        = 0x50020002,
    ERR_WRITE_FAILED       = 0x50020003,
    ERR_RANGE_OUT_OF_BOUNDS = 0x50020004,
    ERR_CANCELLED          = 0x50020005,
    ERR_RETRY_EXHAUSTED    = 0x50020006,

    /* --- format / parse --- */
    ERR_BAD_MAGIC          = 0x50030001,
    ERR_BAD_CHECKSUM       = 0x50030002,
    ERR_BAD_PARAMETER      = 0x50030003,
    ERR_TRUNCATED          = 0x50030004,
    ERR_FIXUP_FAILED       = 0x50030005,
    ERR_CYCLE_LIMIT        = 0x50030006,
    ERR_DEPTH_LIMIT        = 0x50030007,
    ERR_UNSUPPORTED        = 0x50030008,
    ERR_NO_MEMORY          = 0x50030009,

    /* --- session --- */
    ERR_SESSION_FORMAT     = 0x50040001,
    ERR_SESSION_IO         = 0x50040002,
    ERR_SESSION_VERSION    = 0x50040003,

    /* --- recovery writer / policy --- */
    ERR_DEST_OVERLAP       = 0x50050001,
    ERR_DEST_UNWRITABLE    = 0x50050002,
    ERR_HASH_MISMATCH      = 0x50050003,

    /* --- bitlocker policy --- */
    ERR_FVE_LOCKED         = 0x50060001,
};

/* Human-readable text for codes used in CLI output and logs. */
const char *err_text(u32 code);

} // namespace cr

#endif // CR_ERROR_H_
