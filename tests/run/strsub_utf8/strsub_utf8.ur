(* strsubUtf8 gives the bytes of a string's UTF-8 encoding as chars, each a
   code point from 0 to 255; one of them also goes into a URL and back. *)

fun show1 (c : char) = return <xml><body>{[ord c]}</body></xml>

fun main () = return <xml><body>
  <p>{[ord (strsubUtf8 "a" 0)]} {[ord (strsubUtf8 "é" 0)]} {[ord (strsubUtf8 "é" 1)]}</p>
  <a link={show1 (strsubUtf8 "é" 0)}>first byte of e-acute</a>
</body></xml>
