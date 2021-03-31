/*
 * The Shadow Simulator
 * See LICENSE for licensing information
 */

#include "main/host/syscall/shadow.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "main/core/worker.h"
#include "main/host/syscall/protected.h"
#include "main/host/syscall_types.h"
#include "main/host/thread_ptrace.h"
#include "main/routing/address.h"
#include "main/shmem/shmem_allocator.h"
#include "support/logger/logger.h"

SysCallReturn syscallhandler_shadow_hostname_to_addr_ipv4(SysCallHandler* sys,
                                                          const SysCallArgs* args) {
    utility_assert(sys && args);
    PluginPtr name_ptr = args->args[0].as_ptr;
    size_t name_len = args->args[1].as_u64;
    PluginPtr addr_ptr = args->args[2].as_ptr;
    size_t addr_len = args->args[3].as_u64;

    debug("Handling custom syscall shadow_hostname_to_addr_ipv4");

    if (!name_ptr.val || !addr_ptr.val || addr_len < sizeof(uint32_t)) {
        debug("Invalid argument detected, returning EINVAL");
        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = -EINVAL};
    }

    const char* name = process_getReadablePtr(sys->process, sys->thread, name_ptr, name_len);

    debug("Looking up name %s", name);
    Address* address = worker_resolveNameToAddress(name);

    if (address) {
        debug("Found address %s for name %s", address_toString(address), name);

        uint32_t ip = address_toNetworkIP(address);
        uint32_t* addr = process_getWriteablePtr(sys->process, sys->thread, addr_ptr, addr_len);
        *addr = ip;

        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = 0};
    } else {
        debug("Unable to find address for name %s", name);
        // return EFAULT like gethostname
        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = -EFAULT};
    }
}

SysCallReturn syscallhandler_shadow_set_ptrace_allow_native_syscalls(SysCallHandler* sys,
                                                                     const SysCallArgs* args) {
    utility_assert(sys && args);

    InterposeMethod imethod = process_getInterposeMethod(sys->process);

    if (imethod == INTERPOSE_PTRACE || imethod == INTERPOSE_HYBRID) {
        bool is_allowed = args->args[0].as_i64;
        debug("shadow_set_ptrace_allow_native_syscalls is_allowed=%d", is_allowed);

        threadptrace_setAllowNativeSyscalls(sys->thread, is_allowed);

        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = 0};
    } else {
        debug("shadow_set_ptrace_allow_native_syscalls not supported for interpose method %d",
              (int)imethod);
        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = 0};
    }
}

static SysCallReturn _syscallhandler_get_shmem_block(SysCallHandler* sys, const SysCallArgs* args,
                                                     ShMemBlock* block) {
    if (!block) {
        return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = -ENODEV};
    }

    PluginPtr shm_blk_pptr = args->args[0].as_ptr;

    ShMemBlockSerialized* shm_blk_ptr =
        process_getWriteablePtr(sys->process, sys->thread, shm_blk_pptr, sizeof(*shm_blk_ptr));
    *shm_blk_ptr = shmemallocator_globalBlockSerialize(block);

    return (SysCallReturn){.state = SYSCALL_DONE, .retval.as_i64 = 0};
}

SysCallReturn syscallhandler_shadow_get_ipc_blk(SysCallHandler* sys, const SysCallArgs* args) {
    utility_assert(sys && args);
    debug("handling shadow_get_ipc_blk syscall");
    return _syscallhandler_get_shmem_block(sys, args, thread_getIPCBlock(sys->thread));
}

SysCallReturn syscallhandler_shadow_get_shm_blk(SysCallHandler* sys, const SysCallArgs* args) {
    utility_assert(sys && args);
    debug("handling shadow_get_shm_blk syscall");
    return _syscallhandler_get_shmem_block(sys, args, thread_getShMBlock(sys->thread));
}