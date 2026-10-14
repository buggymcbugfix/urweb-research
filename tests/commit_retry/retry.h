#include <urweb.h>
uw_Basis_int uw_Retry_step(uw_context);
uw_Basis_int uw_Retry_attempts(uw_context);
uw_unit uw_Retry_arm(uw_context);
uw_Basis_bool uw_Retry_inTxn(uw_context);
uw_unit uw_Retry_watch(uw_context, uw_Basis_string);
uw_unit uw_Retry_watchAfter(uw_context, uw_Basis_string);
