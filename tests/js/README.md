Client-side code
================

The snapshot is `client.js`, the program's own client-side script, the one
served at `app.<sha1>.js`: its URL rules, its compiled functions, its time
format and whatever `jsFile` directives pull in.  The runtime,
`lib/js/urweb.js`, is served as a file of its own and is not the compiler's
work, so it does not appear here.

Each function is labelled with the span it came from and numbered within
the script, from one, in the order the compiler emits them:

    // counter.ur:4:6-4:77
    urfuncs[1] = {c:"t",f:'{c:"l",b:...}'};

Both matter for reading a diff.  The numbers used to be the compiler's
global name indices, so anything added to the standard library shifted
every one of them and the whole snapshot moved.  The other numbers in a
script, constructor tags and decoder names, and all the names in the C
came from the same counter.  Since the `renumber` pass the constructor
tags and the C names are numbered per program, from one, in declaration
order (the C once more just before it is printed, so that what shaking
removed after the script was made leaves no gaps), and the decoders are
numbered within the script, from one, in the order they are first needed;
a program's output only changes when the program does.  A span survives
everything, and says where to look.

The `recursion` case has a function that calls itself, so the numbering is
exercised where it is easiest to get wrong: a function needs its number
before its body is compiled.  `serverOnly` has no client-side code at all,
and its empty snapshot says so; the day it stops being empty, something
started needing a script that did not before.

The `datatype` case constructs, matches and decodes a datatype on the
client.  What the script carries for a constructor is its number
(`{c:"1",n:3,v:...}`), and the decoder for the RPC's answer is `_n1`; the
case also tracks `server.c`, so that the same program shows the numbering
in both outputs.
