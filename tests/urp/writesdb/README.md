The writesDB directive, and the writesDB mode of an ffi declaration: an FFI
function that writes the database through the application's own
connection, which the compiler cannot see.  In both and bothinline the
function is declared benignEffectful as well, which is refused.
