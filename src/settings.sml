(* Copyright (c) 2008-2011, Adam Chlipala
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - The names of contributors may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *)

structure Settings :> SETTINGS = struct

val configBin = ref Config.bin
val configLib = ref Config.lib
val configSrcLib = ref Config.srclib
val configInclude = ref Config.includ
val configSitelisp = ref Config.sitelisp
val configIcuIncludes = ref Config.icuIncludes
val configIcuLibs = ref Config.icuLibs
val configCCompiler = ref Config.ccompiler

fun getCCompiler () = !configCCompiler
fun setCCompiler cc = configCCompiler := cc

fun libUr () = OS.Path.joinDirFile {dir = !configSrcLib,
                                    file = "ur"}
fun libC () = OS.Path.joinDirFile {dir = !configSrcLib,
                                   file = "c"}
fun libJs () = OS.Path.joinDirFile {dir = !configSrcLib,
                                    file = "js"}

fun libFile s = OS.Path.joinDirFile {dir = libUr (),
                                     file = s}

val urlPrefixFull = ref "/"
val urlPrefix = ref "/"
val urlPrePrefix = ref ""
val timeout = ref 0
val headers = ref ([] : string list)
val scripts = ref ([] : string list)

fun getUrlPrefixFull () = !urlPrefixFull
fun getUrlPrefix () = !urlPrefix
fun getUrlPrePrefix () = !urlPrePrefix
fun setUrlPrefix p =
    let
        val prefix = if p = "" then
                         "/"
                     else if String.sub (p, size p - 1) <> #"/" then
                         p ^ "/"
                     else
                         p

        fun findPrefix n =
            let
                val (befor, after) = Substring.splitl (fn ch => ch <> #"/") (Substring.extract (prefix, n, NONE))
            in
                if Substring.isEmpty after then
                    ("", prefix)
                else
                    (String.substring (prefix, 0, n) ^ Substring.string befor, Substring.string after)
            end

        val (prepre, prefix) =
            if String.isPrefix "http://" prefix then
                findPrefix 7
            else if String.isPrefix "https://" prefix then
                findPrefix 8
            else
                ("", prefix)
    in
        urlPrefixFull := p;
        urlPrePrefix := prepre;
        urlPrefix := prefix
    end

fun getTimeout () = !timeout
fun setTimeout n = timeout := n

fun getHeaders () = !headers
fun setHeaders ls = headers := ls

fun getScripts () = !scripts
fun setScripts ls = scripts := ls

type ffi = string * string

structure K = struct
type ord_key = ffi
fun compare ((m1, x1), (m2, x2)) =
    Order.join (String.compare (m1, m2),
             fn () => String.compare (x1, x2))
end

structure S = BinarySetFn(K)
structure M = BinaryMapFn(K)

fun basis x = S.addList (S.empty, map (fn x : string => ("Basis", x)) x)

val clientToServerBase = basis ["int",
                                "float",
                                "string",
                                "char",
                                "time",
                                "file",
                                "unit",
                                "option",
                                "list",
                                "bool",
                                "variant"]
val clientToServer = ref clientToServerBase
fun setClientToServer ls = clientToServer := S.addList (clientToServerBase, ls)
fun mayClientToServer x = S.member (!clientToServer, x)

val effectfulBase = basis ["dml",
                           "nextval",
                           "setval",
                           "set_cookie",
                           "clear_cookie",
                           "new_channel",
                           "send",
                           "htmlifyInt_w",
                           "htmlifyFloat_w",
                           "htmlifyString_w",
                           "htmlifyBool_w",
                           "htmlifyTime_w",
                           "attrifyInt_w",
                           "attrifyFloat_w",
                           "attrifyString_w",
                           "attrifyChar_w",
                           "urlifyInt_w",
                           "urlifyFloat_w",
                           "urlifyString_w",
                           "urlifyBool_w",
                           "urlifyChannel_w"]

val effectful = ref effectfulBase
fun setEffectful ls = effectful := S.addList (effectfulBase, ls)
fun isEffectful ("Sqlcache", _) = true
  | isEffectful x = S.member (!effectful, x)
fun addEffectful x = effectful := S.add (!effectful, x)

val benignBase = basis ["get_cookie",
                        "getenv",
                        "new_client_source",
                        "get_client_source",
                        "set_client_source",
                        "current",
                        "alert",
                        "confirm",
                        "reload",
                        "onError",
                        "onFail",
                        "onConnectFail",
                        "onDisconnect",
                        "onServerError",
                        "mouseEvent",
                        "keyEvent",
                        "debug",
                        "rand",
                        "now",
                        "getHeader",
                        "setHeader",
                        "spawn",
                        "onClick",
                        "onDblclick",
                        "onContextmenu",
                        "onKeydown",
                        "onKeypress",
                        "onKeyup",
                        "onMousedown",
                        "onMouseenter",
                        "onMouseleave",
                        "onMousemove",
                        "onMouseout",
                        "onMouseover",
                        "onMouseup",
                        "preventDefault",
                        "stopPropagation",
                        "fresh",
                        "giveFocus",
                        "currentUrlHasPost",
                        "currentUrlHasQueryString",
                        "currentUrl"]

val benign = ref benignBase
fun setBenignEffectful ls = benign := S.addList (benignBase, ls)
fun addBenignEffectful x = benign := S.add (!benign, x)
fun isBenignEffectful x = S.member (!benign, x)

val clientBase = basis ["get_client_source",
                        "current",
                        "alert",
                        "confirm",
                        "reload",
                        "recv",
                        "sleep",
                        "spawn",
                        "onError",
                        "onFail",
                        "onConnectFail",
                        "onDisconnect",
                        "onServerError",
                        "mouseEvent",
                        "keyEvent",
                        "onClick",
                        "onContextmenu",
                        "onDblclick",
                        "onKeydown",
                        "onKeypress",
                        "onKeyup",
                        "onMousedown",
                        "onMouseenter",
                        "onMouseleave",
                        "onMousemove",
                        "onMouseout",
                        "onMouseover",
                        "onMouseup",
                        "preventDefault",
                        "stopPropagation",
                        "giveFocus"]
val client = ref clientBase
fun setClientOnly ls = client := S.addList (clientBase, ls)
fun addClientOnly x = client := S.add (!client, x)
fun isClientOnly x = S.member (!client, x)

val serverBase = basis ["requestHeader",
                        "query",
                        "dml",
                        "nextval",
                        "setval",
                        "channel",
                        "send",
                        "fieldName",
                        "fieldValue",
                        "remainingFields",
                        "firstFormField"]
val server = ref serverBase
fun setServerOnly ls = server := S.addList (serverBase, ls)
fun addServerOnly x = server := S.add (!server, x)
fun isServerOnly x = S.member (!server, x)

val basisM = foldl (fn ((k, v : string), m) => M.insert (m, ("Basis", k), v)) M.empty

val jsFuncsBase = basisM [("alert", "alert"),
                          ("stringToTime", "stringToTime"),
                          ("stringToTime_error", "stringToTime_error"),
                          ("timef", "strftime"),
                          ("confirm", "confrm"),
                          ("reload", "reload"),
                          ("get_client_source", "sg"),
                          ("current", "scur"),
                          ("htmlifyBool", "bs"),
                          ("htmlifyFloat", "ts"),
                          ("htmlifyInt", "ts"),
                          ("htmlifyString", "eh"),
                          ("new_client_source", "sc"),
                          ("set_client_source", "sv"),
                          ("stringToFloat", "pflo"),
                          ("stringToInt", "pio"),
                          ("stringToFloat_error", "pfl"),
                          ("stringToInt_error", "pi"),
                          ("urlifyInt", "ts"),
                          ("urlifyFloat", "ts"),
                          ("urlifyTime", "ts"),
                          ("urlifyString", "uf"),
                          ("urlifyChar", "uf"),
                          ("urlifyBool", "ub"),
                          ("recv", "rv"),
                          ("strcat", "cat"),
                          ("intToString", "ts"),
                          ("floatToString", "ts"),
                          ("charToString", "ts"),
                          ("onError", "onError"),
                          ("onFail", "onFail"),
                          ("onConnectFail", "onConnectFail"),
                          ("onDisconnect", "onDisconnect"),
                          ("onServerError", "onServerError"),
                          ("attrifyString", "atr"),
                          ("attrifyInt", "ts"),
                          ("attrifyFloat", "ts"),
                          ("attrifyBool", "bs"),
                          ("boolToString", "bs"),
                          ("str1", "id"),
                          ("strsub", "sub"),
                          ("strsubUtf8", "subUtf8"),
                          ("strsuffixUtf8", "sufUtf8"),
                          ("strsuffix", "suf"),
                          ("strlenUtf8", "slenUtf8"),
                          ("strlen", "slen"),
                          ("strindex", "sidx"),
                          ("strsindex", "ssidx"),
                          ("strchr", "schr"),
                          ("substring", "ssub"),
                          ("strcspn", "sspn"),
                          ("strlenGe", "strlenGe"),
                          ("mouseEvent", "uw_mouseEvent"),
                          ("keyEvent", "uw_keyEvent"),
                          ("minTime", "0"),
                          ("stringToBool_error", "s2be"),
                          ("stringToBool", "s2b"),
                          ("stringToChar_error", "s2ce"),
                          ("stringToChar", "s2c"),

                          ("islower", "isLower"),
                          ("isupper", "isUpper"),
                          ("isalpha", "isAlpha"),
                          ("isdigit", "isDigit"),
                          ("isalnum", "isAlnum"),
                          ("isblank", "isBlank"),
                          ("isspace", "isSpace"),
                          ("isxdigit", "isXdigit"),
                          ("isprint", "isPrint"),
                          ("tolower", "toLower"),
                          ("toupper", "toUpper"),
                          ("ord", "ord"),

                          ("checkUrl", "checkUrl"),
                          ("anchorUrl", "anchorUrl"),
                          ("bless", "bless"),
                          ("blessData", "blessData"),
                          ("currentUrl", "currentUrl"),

                          ("eq_time", "eq"),
                          ("lt_time", "lt"),
                          ("le_time", "le"),

                          ("debug", "uw_debug"),
                          ("naughtyDebug", "uw_debug"),

                          ("floatFromInt", "float"),
                          ("ceil", "ceil"),
                          ("trunc", "trunc"),
                          ("round", "round"),
                          ("floor", "floor"),

                          ("pow", "pow"),
                          ("sqrt", "sqrt"),
                          ("sin", "sin"),
                          ("cos", "cos"),
                          ("log", "log"),
                          ("exp", "exp"),
                          ("asin", "asin"),
                          ("acos", "acos"),
                          ("atan", "atan"),
                          ("atan2", "atan2"),
                          ("abs", "abs"),

                          ("now", "now"),
                          ("timeToString", "showTime"),
                          ("htmlifyTime", "showTimeHtml"),
                          ("toSeconds", "toSeconds"),
                          ("addSeconds", "addSeconds"),
                          ("diffInSeconds", "diffInSeconds"),
                          ("toMilliseconds", "toMilliseconds"),
                          ("fromMilliseconds", "fromMilliseconds"),
                          ("diffInMilliseconds", "diffInMilliseconds"),

                          ("fromDatetime", "fromDatetime"),
                          ("datetimeYear", "datetimeYear"),
                          ("datetimeMonth", "datetimeMonth"),
                          ("datetimeDay", "datetimeDay"),
                          ("datetimeHour", "datetimeHour"),
                          ("datetimeMinute", "datetimeMinute"),
                          ("datetimeSecond", "datetimeSecond"),
                          ("datetimeDayOfWeek", "datetimeDayOfWeek"),


                          ("onClick", "uw_onClick"),
                          ("onContextmenu", "uw_onContextmenu"),
                          ("onDblclick", "uw_onDblclick"),
                          ("onKeydown", "uw_onKeydown"),
                          ("onKeypress", "uw_onKeypress"),
                          ("onKeyup", "uw_onKeyup"),
                          ("onMousedown", "uw_onMousedown"),
                          ("onMouseenter", "uw_onMouseenter"),
                          ("onMouseleave", "uw_onMouseleave"),
                          ("onMousemove", "uw_onMousemove"),
                          ("onMouseout", "uw_onMouseout"),
                          ("onMouseover", "uw_onMouseover"),
                          ("onMouseup", "uw_onMouseup"),
                          ("preventDefault", "uw_preventDefault"),
                          ("stopPropagation", "uw_stopPropagation"),

                          ("fresh", "fresh"),

                          ("atom", "atom"),
                          ("css_url", "css_url"),
                          ("property", "property"),
                          ("giveFocus", "giveFocus"),

                          ("htmlifySpecialChar", "htmlifySpecialChar"),
                          ("chr", "chr")]
val jsFuncs = ref jsFuncsBase
val jsModule = ref (NONE : string option)
fun setJsModule m = jsModule := m
fun jsFuncName f =
    case !jsModule of
        SOME m => m ^ "." ^ f
      | NONE => f
(* The .urp parser has already applied each file's jsModule to these names. *)
fun setJsFuncs ls = jsFuncs := foldl (fn ((k, v), m) => M.insert (m, k, v)) jsFuncsBase ls
fun jsFunc x = M.find (!jsFuncs, x)
fun addJsFunc (k, v) = jsFuncs := M.insert (!jsFuncs, k, jsFuncName v)
fun allJsFuncs () = M.listItemsi (!jsFuncs)

datatype pattern_kind = Exact | Prefix
datatype action = Allow | Deny
type rule = { action : action, kind : pattern_kind, pattern : string }

datatype path_kind = Any | Url | Table | Sequence | View | Relation | Cookie | Style
type rewrite = { pkind : path_kind, kind : pattern_kind, from : string, to : string, hyphenate : bool }

fun pak2s pak =
    case pak of
        Exact => "Exact"
      | Prefix => "Prefix"
fun pk2s pk =
    case pk of
        Any => "Any"
      | Url => "Url"
      | Table => "Table"
      | Sequence => "Sequence"
      | View => "View"
      | Relation => "Relation"
      | Cookie => "Cookie"
      | Style => "Style"
fun r2s (r : rewrite) = pak2s (#kind r) ^ " " ^ pk2s (#pkind r) ^ ", from<" ^ #from r ^ ">, to<" ^ #to r ^ ">"

val rewrites = ref ([] : rewrite list)

fun subsume (pk1, pk2) =
    pk1 = pk2
    orelse pk2 = Any
    orelse pk2 = Relation andalso (pk1 = Table orelse pk1 = Sequence orelse pk1 = View)

fun setRewriteRules ls = rewrites := ls
fun rewrite pk s =
    let
        fun rew (ls : rewrite list) =
            case ls of
                [] => s
              | rewr :: ls =>
                let
                    fun match () =
                        case #kind rewr of
                            Exact => if #from rewr = s then
                                         SOME (size s)
                                     else
                                         NONE
                          | Prefix => if String.isPrefix (#from rewr) s then
                                          SOME (size (#from rewr))
                                      else
                                          NONE
                in
                    if subsume (pk, #pkind rewr) then
                        case match () of
                            NONE => rew ls
                          | SOME suffixStart =>
                            let
                                val s = #to rewr ^ String.extract (s, suffixStart, NONE)
                            in
                                if #hyphenate rewr then
                                    String.translate (fn #"_" => "-" | ch => str ch) s
                                else
                                    s
                            end
                    else
                        rew ls
                end
    in
        rew (!rewrites)
    end

val url = ref ([] : rule list)
val mime = ref ([] : rule list)
val request = ref ([] : rule list)
val response = ref ([] : rule list)
val env = ref ([] : rule list)
val meta = ref ([] : rule list)

fun setUrlRules ls = url := ls
fun setMimeRules ls = mime := ls
fun setRequestHeaderRules ls = request := ls
fun setResponseHeaderRules ls = response := ls
fun setEnvVarRules ls = env := ls
fun setMetaRules ls = meta := ls

fun getUrlRules () = !url
fun getMimeRules () = !mime
fun getRequestHeaderRules () = !request
fun getResponseHeaderRules () = !response
fun getEnvVarRules () = !env
fun getMetaRules () = !meta

fun check f rules s =
    let
        fun chk (ls : rule list) =
            case ls of
                [] => false
              | rule :: ls =>
                let
                    val matches =
                        case #kind rule of
                            Exact => #pattern rule = s
                          | Prefix => String.isPrefix (#pattern rule) s
                in
                    if matches then
                        case #action rule of
                            Allow => true
                          | Deny => false
                    else
                        chk ls
                end
    in
        f s andalso chk (!rules)
    end

val checkUrl = check (fn _ => true) url

val validMime = CharVector.all (fn ch => Char.isAlphaNum ch orelse ch = #"/" orelse ch = #"-" orelse ch = #"." orelse ch = #"+")
val validEnv = CharVector.all (fn ch => Char.isAlphaNum ch orelse ch = #"_" orelse ch = #".")
val validMeta = CharVector.all (fn ch => Char.isAlpha ch orelse ch = #"-")

val checkMime = check validMime mime
val checkRequestHeader = check validMime request
val checkResponseHeader = check validMime response
val checkEnvVar = check validEnv env
val checkMeta = check validMeta meta


type protocol = {
     name : string,
     compile : string,
     linkStatic : string,
     linkDynamic : string,
     persistent : bool,
     code : unit -> Print.PD.pp_desc
}
val protocols = ref ([] : protocol list)
fun addProtocol p = protocols := p :: !protocols
fun getProtocol s = List.find (fn p => #name p = s) (!protocols)

fun clibFile s = OS.Path.joinDirFile {dir = libC (),
                                      file = s}

val curProto = ref {name = "",
                    compile = "",
                    linkStatic = "",
                    linkDynamic = "",
                    persistent = false,
                    code = fn () => Print.box []}
fun setProtocol name =
    case getProtocol name of
        NONE => raise Fail ("Unknown protocol " ^ name)
      | SOME p => curProto := p
fun currentProtocol () = !curProto

val debug = ref false
fun setDebug b = debug := b
fun getDebug () = !debug

val dev = ref false
fun setDev b = dev := b
fun getDev () = !dev

datatype sql_type =
         Int
       | Float
       | String
       | Char
       | Bool
       | Time
       | Blob
       | Channel
       | Client
       | Nullable of sql_type

fun p_sql_ctype t =
    let
        open Print.PD
        open Print
    in
        case t of
            Int => "uw_Basis_int"
          | Float => "uw_Basis_float"
          | String => "uw_Basis_string"
          | Char => "uw_Basis_char"
          | Bool => "uw_Basis_bool"
          | Time => "uw_Basis_time"
          | Blob => "uw_Basis_blob"
          | Channel => "uw_Basis_channel"
          | Client => "uw_Basis_client"
          | Nullable String => "uw_Basis_string"
          | Nullable t => p_sql_ctype t ^ "*"
    end

fun isBlob Blob = true
  | isBlob (Nullable t) = isBlob t
  | isBlob _ = false

fun isNotNull (Nullable _) = false
  | isNotNull _ = true

datatype failure_mode = Error | None

type dbms = {
     name : string,
     randomFunction : string,
     header : string,
     link : string,
     p_sql_type : sql_type -> string,
     init : {dbstring : string,
             prepared : (string * int) list,
             tables : (string * (string * sql_type) list) list,
             views : (string * (string * sql_type) list) list,
             sequences : string list} -> Print.PD.pp_desc,
     query : {loc : ErrorMsg.span, cols : sql_type list,
              doCols : ({loc : ErrorMsg.span, wontLeakStrings : bool, col : int, typ : sql_type} -> Print.PD.pp_desc)
                       -> Print.PD.pp_desc}
             -> Print.PD.pp_desc,
     queryPrepared : {loc : ErrorMsg.span, id : int, query : string,
                      inputs : sql_type list, cols : sql_type list,
                      doCols : ({loc : ErrorMsg.span, wontLeakStrings : bool, col : int,
                                 typ : sql_type} -> Print.PD.pp_desc)
                               -> Print.PD.pp_desc,
                      nested : bool}
                     -> Print.PD.pp_desc,
     dml : ErrorMsg.span * failure_mode -> Print.PD.pp_desc,
     dmlPrepared : {loc : ErrorMsg.span, id : int, dml : string,
                    inputs : sql_type list, mode : failure_mode} -> Print.PD.pp_desc,
     nextval : {loc : ErrorMsg.span, seqName : string option, seqE : Print.PD.pp_desc} -> Print.PD.pp_desc,
     nextvalPrepared : {loc : ErrorMsg.span, id : int, query : string} -> Print.PD.pp_desc,
     setval : {loc : ErrorMsg.span, seqE : Print.PD.pp_desc, count : Print.PD.pp_desc} -> Print.PD.pp_desc,
     sqlifyString : string -> string,
     p_cast : string * sql_type -> string,
     p_blank : int * sql_type -> string,
     supportsDeleteAs : bool,
     supportsUpdateAs : bool,
     createSequence : string -> string,
     textKeysNeedLengths : bool,
     supportsNextval : bool,
     supportsNestedPrepared : bool,
     sqlPrefix : string,
     supportsOctetLength : bool,
     trueString : string,
     falseString : string,
     onlyUnion : bool,
     nestedRelops : bool,
     windowFunctions: bool,
     requiresTimestampDefaults : bool,
     supportsIsDistinctFrom : bool,
     supportsSHA512 : {InitializeDb : string, GenerateHash : string -> string} option,
     supportsSimilar : {InitializeDb : string} option,
     keywords : string list,
     identifierQuote : string
}

val dbmses = ref ([] : dbms list)
val curDb = ref ({name = "",
                  randomFunction = "",
                  header = "",
                  link = "",
                  p_sql_type = fn _ => "",
                  init = fn _ => Print.box [],
                  query = fn _ => Print.box [],
                  queryPrepared = fn _ => Print.box [],
                  dml = fn _ => Print.box [],
                  dmlPrepared = fn _ => Print.box [],
                  nextval = fn _ => Print.box [],
                  nextvalPrepared = fn _ => Print.box [],
                  setval = fn _ => Print.box [],
                  sqlifyString = fn s => s,
                  p_cast = fn _ => "",
                  p_blank = fn _ => "",
                  supportsDeleteAs = false,
                  supportsUpdateAs = false,
                  createSequence = fn _ => "",
                  textKeysNeedLengths = false,
                  supportsNextval = false,
                  supportsNestedPrepared = false,
                  sqlPrefix = "",
                  supportsOctetLength = false,
                  trueString = "",
                  falseString = "",
                  onlyUnion = false,
                  nestedRelops = false,
                  windowFunctions = false,
                  requiresTimestampDefaults = false,
                  supportsIsDistinctFrom = false,
                  supportsSHA512 = NONE,
                  supportsSimilar = NONE,
                  keywords = [],
                  identifierQuote = "\""} : dbms)

fun addDbms v = dbmses := v :: !dbmses
fun setDbms s =
    case List.find (fn db => #name db = s) (!dbmses) of
        NONE => raise Fail ("Unknown DBMS " ^ s)
      | SOME db => curDb := db
fun currentDbms () = !curDb
val dbmsChoice = ref (NONE : string option)
fun setDbmsChoice so = (Option.app setDbms so; dbmsChoice := so)
fun getDbmsChoice () = !dbmsChoice

val dbstring = ref (NONE : string option)
fun setDbstring so = dbstring := so
fun getDbstring () = !dbstring

val exe = ref (NONE : string option)
fun setExe so = exe := so
fun getExe () = !exe

val sql = ref (NONE : string option)
fun setSql so = sql := so
fun getSql () = !sql

val endpoints = ref (NONE : string option)
fun setEndpoints so = endpoints := so
fun getEndpoints () = !endpoints

val coreInline = ref 5
fun setCoreInline n = coreInline := n
fun getCoreInline () = !coreInline

val monoInline = ref 5
fun setMonoInline n = monoInline := n
fun getMonoInline () = !monoInline

val staticLinking = ref false
fun setStaticLinking b = staticLinking := b
fun getStaticLinking () = !staticLinking

val bootLinking = ref false
fun setBootLinking b = bootLinking := b
fun getBootLinking () = !bootLinking

val deadlines = ref false
fun setDeadlines b = deadlines := b
fun getDeadlines () = !deadlines

val sigFile = ref (NONE : string option)
fun setSigFile v = sigFile := v
fun getSigFile () = !sigFile

val fileCache = ref (NONE : string option)
fun setFileCache v =
    (if Option.isSome v andalso (case #supportsSHA512 (currentDbms ()) of NONE => true
                                                                        | SOME _ => false) then
         ErrorMsg.error "The selected database engine is incompatible with file caching."
     else
        ();
     fileCache := v)
fun getFileCache () = !fileCache

structure SS = BinarySetFn(struct
                           type ord_key = string
                           val compare = String.compare
                           end)

val safeGetDefault = ref false
val safeGet = ref SS.empty
fun setSafeGetDefault b = safeGetDefault := b
fun setSafeGets ls = safeGet := SS.addList (SS.empty, ls)
fun isSafeGet x = !safeGetDefault orelse SS.member (!safeGet, x)

val onError = ref (NONE : (string * string list * string) option)
fun setOnError x = onError := x
fun getOnError () = !onError

val limits = ["messages", "clients", "headers", "page", "heap", "script",
              "inputs", "subinputs", "cleanup", "deltas", "transactionals",
              "globals", "database", "time"]

val limitsList = ref ([] : (string * int) list)
fun addLimit (v as (name, _)) =
    if List.exists (fn name' => name' = name) limits then
        (limitsList := v :: !limitsList;
         if name = "time" then
             setDeadlines true
         else
             ())
    else
        raise Fail ("Unknown limit category '" ^ name ^ "'")
fun limits () = !limitsList

val minHeap = ref 0
fun setMinHeap n = if n >= 0 then minHeap := n else raise Fail "Trying to set negative minHeap"
fun getMinHeap () = !minHeap

val alwaysInline = ref SS.empty
fun addAlwaysInline s = alwaysInline := SS.add (!alwaysInline, s)
fun checkAlwaysInline s = SS.member (!alwaysInline, s)

val neverInline = ref SS.empty
fun addNeverInline s = neverInline := SS.add (!neverInline, s)
fun checkNeverInline s = SS.member (!neverInline, s)

val noXsrfProtection = ref SS.empty
fun addNoXsrfProtection s = noXsrfProtection := SS.add (!noXsrfProtection, s)
fun checkNoXsrfProtection s = SS.member (!noXsrfProtection, s)

val timeFormat = ref "%c"
fun setTimeFormat v = timeFormat := v
fun getTimeFormat () = !timeFormat

fun lowercase s =
    case s of
        "" => ""
      | _ => str (Char.toLower (String.sub (s, 0))) ^ String.extract (s, 1, NONE)

fun capitalize s =
    case s of
        "" => ""
      | _ => str (Char.toUpper (String.sub (s, 0))) ^ String.extract (s, 1, NONE)

val allLower = CharVector.map Char.toLower

val mangle = ref true
fun setMangleSql x = mangle := x

(* Is [s] (in any letter case) a keyword of the current DBMS, i.e. a word it
 * will not accept unquoted as an identifier?  Each backend lists its reserved
 * words in the [keywords] field of its [dbms] record; the set is built once
 * per DBMS. *)
local
    val cache = ref ("", SS.empty)
in
fun isSqlKeyword s =
    let
        val db = currentDbms ()
        val (name, set) = !cache
        val set =
            if name = #name db then
                set
            else
                let
                    val set = foldl SS.add' SS.empty (#keywords db)
                in
                    cache := (#name db, set);
                    set
                end
    in
        SS.member (set, CharVector.map Char.toUpper s)
    end
end

(* Can [s] be used unquoted as an SQL identifier?  A plain identifier starts
 * with a letter or underscore and continues with letters, digits and
 * underscores.  Ur produces names outside that form, e.g. "1" for an
 * anonymous SELECT column, and Ur names may clash with keywords, e.g. a
 * table called `order` or a field called `Select`. *)
fun sqlNeedsQuoting s =
    case String.explode s of
        [] => true
      | ch :: rest =>
        not (Char.isAlpha ch orelse ch = #"_")
        orelse not (List.all (fn ch => Char.isAlphaNum ch orelse ch = #"_") rest)
        orelse isSqlKeyword s

(* Render an unmangled name as an SQL identifier: as is, or quoted with the
 * DBMS's delimiter when it could not stand on its own.  Quoting is applied
 * consistently everywhere the identifier is rendered -- schema, queries,
 * constraint text -- so the quoted and unquoted spellings never need to be
 * reconciled. *)
fun sqlIdentifier s =
    if sqlNeedsQuoting s then
        let
            val q = #identifierQuote (currentDbms ())
        in
            q ^ s ^ q
        end
    else
        s

(* The bare name inside a possibly-quoted SQL identifier, for catalog lookups
 * and for deriving further names from it. *)
fun unquoteSql s =
    let
        val q = #identifierQuote (currentDbms ())
    in
        if size s >= 2 * size q andalso String.isPrefix q s andalso String.isSuffix q s then
            String.substring (s, size q, size s - 2 * size q)
        else
            s
    end

fun getMangleSql () = !mangle

(* How an unmangled name is spelled, before quoting: MySQL gets all-lowercase
 * names (its table names are case-sensitive on some platforms), Postgres a
 * lowercase first letter (unquoted identifiers fold to lower case anyway),
 * and SQLite the Ur name exactly as written (it compares identifiers
 * case-insensitively, so CamelCase costs nothing). *)
fun unmangledCase s =
    case #name (currentDbms ()) of
        "mysql" => allLower s
      | "sqlite" => s
      | _ => lowercase s

(* Names derived from others (constraint names like tab_pkey, index names like
 * tab_col1_col2) are built from the bare names and then rendered like any
 * other identifier.  Mangled names carry the uw_ prefix and so never need
 * quoting. *)
fun sqlDerivedName parts =
    let
        val s = String.concatWith "_" (map unquoteSql parts)
    in
        if !mangle then
            s
        else
            sqlIdentifier (unmangledCase s)
    end

(* Table and view names.  Mangling prefixes uw_, which also keeps the names
 * clear of SQL keywords; without it, the name is spelled per [unmangledCase]
 * and quoted if the DBMS requires. *)
fun mangleSqlTable s =
    if !mangle then
        case #name (currentDbms ()) of
            "mysql" => "uw_" ^ allLower s
          | _ => "uw_" ^ capitalize s
    else
        sqlIdentifier (unmangledCase s)

(* Column and sequence names, likewise.  Callers pass the Ur name unchanged;
 * any case adjustment belongs here. *)
fun mangleSql s =
    if !mangle then
        case #name (currentDbms ()) of
            "mysql" => "uw_" ^ allLower s
          | _ => "uw_" ^ s
    else
        sqlIdentifier (unmangledCase s)

(* The form of a column name to compare against the database catalog (never
 * quoted). *)
fun mangleSqlCatalog s =
    if !mangle then
        case #name (currentDbms ()) of
            "mysql" => "uw_" ^ allLower s
          | _ => "uw_" ^ s
    else
        unmangledCase s

val html5 = ref true
fun setIsHtml5 b = html5 := b
fun getIsHtml5 () = !html5

val less = ref false
fun setLessSafeFfi b = less := b
fun getLessSafeFfi () = !less

val sqlcache = ref false
fun setSqlcache b = sqlcache := b
fun getSqlcache () = !sqlcache

structure SM = BinaryMapFn(struct
                           type ord_key = string
                           val compare = String.compare
                           end)

val noMimeFile = ref false

val mimeFilePath = ref "/etc/mime.types"
val mimeTypes = ref (NONE : string SM.map option)
(* Changing the path drops the table read from the old one, so that later lookups use the new file. *)
fun setMimeFilePath file =
    if file = !mimeFilePath then
        ()
    else
        (mimeFilePath := file;
         mimeTypes := NONE;
         noMimeFile := false)
fun getMimeFilePath () = !mimeFilePath

fun noMime () =
    (TextIO.output (TextIO.stdErr, "WARNING: Error opening " ^ !mimeFilePath ^ ".  Static files will be served with no suggested MIME types.\n");
     noMimeFile := true;
     SM.empty)

fun readMimeTypes () =
    let
        val inf = FileIO.txtOpenIn (!mimeFilePath)

        fun loop m =
            case TextIO.inputLine inf of
                NONE => m
              | SOME line =>
                if size line > 0 andalso String.sub (line, 0) = #"#" then
                    loop m
                else
                    case String.tokens Char.isSpace line of
                        typ :: exts =>
                        loop (foldl (fn (ext, m) => SM.insert (m, ext, typ)) m exts)
                      | _ => loop m
    in
        loop SM.empty
        before TextIO.closeIn inf
    end handle IO.Io _ => noMime ()
             | OS.SysErr _ => noMime ()

fun getMimeTypes () =
    case !mimeTypes of
        SOME m => m
      | NONE =>
        let
            val m = readMimeTypes ()
        in
            mimeTypes := SOME m;
            m
        end

fun mimeTypeOf filename =
    case OS.Path.ext filename of
        NONE => (if !noMimeFile then
                     ()
                 else
                     TextIO.output (TextIO.stdErr, "WARNING: No extension found in filename '" ^ filename ^ "'.  Header 'Content-Type' will be omitted in HTTP responses.\n");
                 NONE)
      | SOME ext =>
        let
            val to = SM.find (getMimeTypes (), ext)
        in
            case to of
                NONE => if !noMimeFile then
                            ()
                        else
                            TextIO.output (TextIO.stdErr, "WARNING: No MIME type known for extension '" ^ ext ^ "'.  Header 'Content-Type' will be omitted in HTTP responses.\n")
              | _ => ();
            to
        end

val files = ref (SM.empty : (string * {Uri : string, ContentType : string option, LastModified : Time.time, Bytes : Word8Vector.vector}) SM.map)

val filePath = ref "."

fun setFilePath path = filePath := path

fun addFile {Uri, LoadFromFilename, MimeType} =
    let
        val path = OS.Path.concat (!filePath, LoadFromFilename)
                   handle Path => LoadFromFilename
    in
        case SM.find (!files, Uri) of
            SOME (path', _) =>
            if OS.Path.mkCanonical path' = OS.Path.mkCanonical path then
                ()
            else
                ErrorMsg.error ("Two different files requested for URI " ^ Uri ^ " ( " ^ path' ^ " vs. " ^ path ^ ")")
          | NONE =>
            let
                val inf = FileIO.binOpenIn path
            in
                files := SM.insert (!files,
                                    Uri,
                                    (path,
                                     {Uri = Uri,
                                      ContentType = case MimeType of
                                                        NONE => mimeTypeOf path
                                                      | _ => MimeType,
                                      LastModified = OS.FileSys.modTime path,
                                      Bytes = BinIO.inputAll inf}));
                BinIO.closeIn inf
            end
    end handle IO.Io _ =>
               ErrorMsg.error ("Error loading file " ^ LoadFromFilename)
             | OS.SysErr (s, _) =>
               ErrorMsg.error ("Error loading file " ^ LoadFromFilename ^ " (" ^ s ^ ")")

fun listFiles () = map #2 (SM.listItems (!files))

val jsFiles = ref (SM.empty : {Filename : string, Content : string} SM.map)

fun addJsFile LoadFromFilename =
    let
        val path = OS.Path.concat (!filePath, LoadFromFilename)
        val inf = FileIO.txtOpenIn path
    in
        jsFiles := SM.insert (!jsFiles,
                              path,
                              {Filename = LoadFromFilename,
                               Content = TextIO.inputAll inf});
        TextIO.closeIn inf
    end handle IO.Io _ =>
               ErrorMsg.error ("Error loading file " ^ LoadFromFilename)
             | OS.SysErr (s, _) =>
               ErrorMsg.error ("Error loading file " ^ LoadFromFilename ^ " (" ^ s ^ ")")

fun listJsFiles () = SM.listItems (!jsFiles)

val jsOutput = ref (NONE : string option)
fun setOutputJsFile so =
    (case so of
         SOME s =>
         if s = "" orelse CharVector.exists (fn ch => ch = #"/") s then
             raise Fail ("Bad -js argument \"" ^ s ^ "\": expected a plain path component, without \"/\".\n"
                         ^ "The generated JavaScript is embedded in the executable and served at the application prefix followed by this name.")
         else
             ()
       | NONE => ();
     jsOutput := so)
fun getOutputJsFile () = !jsOutput

val saveJs = ref (NONE : string option)
fun setSaveJs so = saveJs := so
fun getSaveJs () = !saveJs

fun reset () =
    (Globals.setResetTime ();
     urlPrefixFull := "/";
     urlPrefix := "/";
     urlPrePrefix := "";
     timeout := 0;
     headers := [];
     scripts := [];
     clientToServer := clientToServerBase;
     effectful := effectfulBase;
     benign := benignBase;
     client := clientBase;
     server := serverBase;
     jsFuncs := jsFuncsBase;
     rewrites := [];
     url := [];
     mime := [];
     request := [];
     response := [];
     env := [];
     meta := [];
     debug := false;
     dev := false;
     dbmsChoice := NONE;
     dbstring := NONE;
     exe := NONE;
     sql := NONE;
     endpoints := NONE;
     coreInline := 5;
     monoInline := 5;
     staticLinking := false;
     deadlines := false;
     sigFile := NONE;
     safeGet := SS.empty;
     onError := NONE;
     limitsList := [];
     minHeap := 0;
     alwaysInline := SS.empty;
     neverInline := SS.empty;
     noXsrfProtection := SS.empty;
     timeFormat := "%c";
     mangle := true;
     html5 := false;
     less := false;
     noMimeFile := false;
     mimeTypes := NONE;
     files := SM.empty;
     jsFiles := SM.empty;
     filePath := ".";
     jsOutput := NONE)

end
