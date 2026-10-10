Declarations that are not pages
===============================

A program's interesting declarations are not all pages.  An `onError`
handler, the tasks (`initialize`, `periodic`, `clientLeaves`), a policy, a
cookie, a channel: none of them is exported, so nothing under `demo/` or
`tests/js` shows what the compiler makes of them, and a change there would
go unnoticed.  The `decls` case has one of each, under sqlite, and tracks
`server.c`: the handler and the tasks appear as functions the runtime calls
by their numbered names (`__uwn_err_3`, `uw_initializer`, `my_periodics`,
`uw_expunger`), the channel makes `main` a push handler in the dispatch
table, and the policy leaves no trace in the C at all, which is also worth
knowing.
