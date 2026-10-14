Where `link FOO.c` finds and puts things: two sources with the same base
name in different directories (a/x.c, b/x.c), one of them named twice, and
a source named by a library's own project file, in another directory
(lib/lib.urp). Each object has to be its own, and each file found where its
project says.
