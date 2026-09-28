#include <urweb.h>

uw_Basis_int uw_Iocheck_step(uw_context);
uw_Basis_bool uw_Iocheck_inTransaction(uw_context);
uw_Basis_bool uw_Iocheck_inTransactionT(uw_context);
uw_unit uw_Iocheck_boundedRetry(uw_context, uw_Basis_string);
uw_unit uw_Iocheck_effect(uw_context, uw_Basis_string);
