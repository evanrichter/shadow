#ifndef LIB_SHIM_SHIM_SIGNALS_H
#define LIB_SHIM_SHIM_SIGNALS_H

#include <stdbool.h>

#include "lib/shim/shim_shmem.h"

// Handle pending unblocked signals, and return whether *all* corresponding
// signal actions had the SA_RESTART flag set.
//
// `ucontext` will be passed through to handlers if non-NULL. This should
// generally only be done if the caller has a `ucontext` that will be swapped to
// after this code returns; e.g. one that was passed to our own signal handler,
// which will be swapped to when that handler returns.
//
// If `ucontext` is NULL, one will be created at the point where we invoke
// the handler, and swapped back to when it returns.
// TODO: Creating `ucontext_t` is currently only implemented for handlers that
// execute on a sigaltstack.
bool shim_process_signals(ShimShmemHostLock* host_lock, ucontext_t* context);

// Install signal handlers for signals that can be generated by hardware errors.
// e.g. SIGSEGV
void shim_install_hardware_error_handlers();

// More-specialized error handlers (e.g. for rdtsc) can invoke this handler
// directly when unable to handle the current signal (e.g. when a SIGSEGV wasn't
// caused by an rdtsc instruction)
void shim_handle_hardware_error_signal(int, siginfo_t*, void*);

#endif