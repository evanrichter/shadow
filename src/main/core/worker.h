/*
 * The Shadow Simulator
 * Copyright (c) 2010-2011, Rob Jansen
 * See LICENSE for licensing information
 */

#ifndef SHD_WORKER_H_
#define SHD_WORKER_H_

#include <glib.h>
#include <netinet/in.h>
#include <stdbool.h>

// A pool of worker threads.
typedef struct _WorkerPool WorkerPool;
// Task to be executed on a worker thread.
typedef void (*WorkerPoolTaskFn)(void*);

#include "lib/logger/log_level.h"
#include "main/core/support/definitions.h"
#include "main/host/host.h"
#include "main/host/syscall_types.h"
#include "main/host/thread.h"
#include "main/routing/address.h"
#include "main/routing/dns.h"
#include "main/routing/packet.minimal.h"
#include "main/utility/count_down_latch.h"

#include "main/bindings/c/bindings.h"

// To be called by worker thread
void worker_finish(GQueue* hosts, CSimulationTime time);

// Create a workerpool with `nThreads` threads, allowing up to `nConcurrent` to
// run at a time.
WorkerPool* workerpool_new(int nWorkers, int nParallel);

// Begin executing taskFn(data) on each worker thread in the pool.
void workerpool_startTaskFn(WorkerPool* pool, WorkerPoolTaskFn taskFn,
                            void* data);
// Await completion of a taskFn on every thread in the pool.
void workerpool_awaitTaskFn(WorkerPool* pool);
int workerpool_getNWorkers(WorkerPool* pool);
// Signal worker threads to exit and wait for them to do so.
void workerpool_joinAll(WorkerPool* pool);
void workerpool_free(WorkerPool* pool);
pthread_t workerpool_getThread(WorkerPool* pool, int threadId);

// Compute the global min event time across all workers. We dynamically compute
// the minimum time that we'll need for the next event round as the minimum of
// i.) all events pushed by all workers during this round, and
// ii.) the next queued event for all worker at the point when they stop
// executing events.
//
// This func is not thread safe, so only call from the scheduler thread when the
// workers are idle.
CSimulationTime workerpool_getGlobalNextEventTime(WorkerPool* workerPool);

// The worker either pushed an event or finished executing its events and is
// reporting the min time of events in their event queue.
void worker_setMinEventTimeNextRound(CSimulationTime simtime);

// When a new scheduling round starts, set the end time of the new round.
void worker_setRoundEndTime(CSimulationTime newRoundEndTime);

int worker_getAffinity();
gboolean worker_scheduleTaskWithDelay(TaskRef* task, Host* host, CSimulationTime nanoDelay);
gboolean worker_scheduleTaskAtEmulatedTime(TaskRef* task, Host* host, CEmulatedTime t);
void worker_sendPacket(Host* src, Packet* packet);
bool worker_isAlive(void);
// Maximum time that the current event may run ahead to. Must only be called if we hold the host
// lock.
CEmulatedTime worker_maxEventRunaheadTime(Host* host);

/* Time from the  beginning of the simulation.
 * Deprecated - prefer `worker_getCurrentEmulatedTime`.
 */
CSimulationTime worker_getCurrentSimulationTime();

/* The emulated time starts at January 1st, 2000. This time should be used
 * in any places where time is returned to the application, to handle code
 * that assumes the world is in a relatively recent time. */
CEmulatedTime worker_getCurrentEmulatedTime();

bool worker_isBootstrapActive(void);

void worker_clearCurrentTime();
void worker_setCurrentEmulatedTime(CEmulatedTime time);

void worker_bootHosts(GQueue* hosts);
void worker_freeHosts(GQueue* hosts);

// Increment a counter for the allocation of the object with the given name.
// This should be paired with an increment of the dealloc counter with the
// same name, otherwise we print a warning that a memory leak was detected.
#define worker_count_allocation(type) worker_increment_object_alloc_counter(#type)

// Increment a counter for the deallocation of the object with the given name.
// This should be paired with an increment of the alloc counter with the
// same name, otherwise we print a warning that a memory leak was detected.
#define worker_count_deallocation(type) worker_increment_object_dealloc_counter(#type)

#endif /* SHD_WORKER_H_ */
