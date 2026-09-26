type t = Basis.string

val str = Basis.str1

val length = Basis.strlen
val lengthGe = Basis.strlenGe
val append = Basis.strcat

val sub = Basis.strsub
val suffix = Basis.strsuffix

val index = Basis.strindex
fun sindex r = Basis.strsindex r.Haystack r.Needle
val atFirst = Basis.strchr

fun mindex {Haystack = s, Needle = chs} =
    let
        val n = Basis.strcspn s chs
    in
        if n >= length s then
            None
        else
            Some n
    end

fun substring s {Start = start, Len = len} = Basis.substring s start len

fun seek s ch =
    case index s ch of
        None => None
      | Some i => Some (suffix s (i + 1))
fun mseek {Haystack = s, Needle = chs} =
    case mindex {Haystack = s, Needle = chs} of
        None => None
      | Some i => Some (sub s i, suffix s (i + 1))

fun split s ch =
    case index s ch of
        None => None
      | Some i => Some (substring s {Start = 0, Len = i},
                        suffix s (i + 1))
fun split' s ch =
    case index s ch of
        None => None
      | Some i => Some (substring s {Start = 0, Len = i},
                        suffix s i)
fun msplit {Haystack = s, Needle = chs} =
    case mindex {Haystack = s, Needle = chs} of
        None => None
      | Some i => Some (substring s {Start = 0, Len = i},
                        sub s i,
                        suffix s (i + 1))

fun ssplit r =
    case sindex r of
        None => None
      | Some i => Some (substring r.Haystack {Start = 0, Len = i},
                        suffix r.Haystack (i + length r.Needle))

(* Indexing a string by code point costs time proportional to the index, on
 * both the server (UTF-8) and the client (UTF-16).  Loops over a string
 * therefore walk it with [suffix], looking only at position 0, rather than
 * indexing from the front on every step, which would be quadratic. *)
fun all f s =
    let
        fun al s =
            s = ""
            || (f (sub s 0) && al (suffix s 1))
    in
        al s
    end

fun mp f s =
    let
        fun mp' s acc =
            if s = "" then
                acc
            else
                mp' (suffix s 1) (acc ^ str (f (sub s 0)))
    in
        mp' s ""
    end

fun newlines [ctx] [[Body] ~ ctx] (s : string) : xml ([Body] ++ ctx) [] [] =
    case split s #"\n" of
        None => cdata s
      | Some (s1, s2) => <xml>{[s1]}<br/>{newlines s2}</xml>

fun isPrefix {Full = f, Prefix = p} =
    let
        val plen = length p
    in
        (* lengthGe avoids measuring all of f when it is much longer than p. *)
        lengthGe f plen && substring f {Start = 0, Len = plen} = p
    end

fun trim s =
    let
        val len = length s

        fun findStart i =
            if i < len && isspace (sub s i) then
                findStart (i+1)
            else
                i

        fun findFinish i =
            if i >= 0 && isspace (sub s i) then
                findFinish (i-1)
            else
                i

        val start = findStart 0
        val finish = findFinish (len - 1)
    in
        if finish >= start then
            substring s {Start = start, Len = finish - start + 1}
        else
            ""
    end
