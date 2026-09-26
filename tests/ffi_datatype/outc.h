#include <urweb.h>

/* The layout the code generator expects for Outc.outcome: the tag enum's
   constants are uw_Outc_<Con>, and a constructor's argument is the union
   member uw_<Con>. */
enum uw_Outc_outcome_tag { uw_Outc_Sent, uw_Outc_NotSent, uw_Outc_Unknown };
struct uw_Outc_outcome {
  enum uw_Outc_outcome_tag tag;
  union { uw_Basis_string uw_NotSent; uw_Basis_string uw_Unknown; } data;
};
typedef struct uw_Outc_outcome *uw_Outc_outcome;

uw_Outc_outcome uw_Outc_send(uw_context, uw_Basis_string);
uw_Basis_string uw_Outc_describe(uw_context, uw_Outc_outcome);
