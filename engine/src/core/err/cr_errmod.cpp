/* cr_errmod.cpp - error-context module attribution (M-049, diff-to-fix).
 *
 * thread_local storage; see cr_errmod.h for the attribution model.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_errmod.h"

namespace cr {
namespace errmod {

namespace {
thread_local u32 t_stamp = kNone;
} // namespace

u32 current() {
    return t_stamp;
}

u32 set_thread(u32 module) {
    const u32 prev = t_stamp;
    t_stamp = module;
    return prev;
}

} // namespace errmod
} // namespace cr
