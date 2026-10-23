#include "queue.h"

/* Functions of the runtime that live in request.o and queue.o, the members
 * of liburweb.a that the generated code does not need. Their addresses are
 * kept in variables so that the references survive the optimiser. */
extern void uw_enqueue(void);
extern void uw_request_init(void);

volatile long keep_enqueue = (long)&uw_enqueue;
volatile long keep_request_init = (long)&uw_request_init;

uw_Basis_int uw_Queue_touch(uw_context ctx, uw_Basis_unit u) {
    return (keep_enqueue ^ keep_request_init) & 1;
}
