# The files an Ur/Web project reads, from its .urp, as src/compiler.sml
# reads it: the modules listed after the directives (each a .ur, and a
# .urs where there is one), the libraries (`library`: a .urp, or a
# directory with a lib.urp, read the same way) and the files the other
# directives name (`file`, `jsFile`, `include`, `ffi`, `link`,
# `mimeTypes`).  A package whose source is exactly these is rebuilt
# only when one of them changes.  buildApp uses it when given `root`.
#
# `$NAME/…` is followed when NAME comes from a `path` directive.  `$/…`
# is the compiler's own library, and any other `$…` a path given on the
# command line (-path, as withLibraries does for NIX_LIBS); both are
# outside the tree and skipped, as are absolute paths.
#
# What is reached only from inside a named file is not seen, a header
# that a header named by `include` includes, say.  Such a file has to be
# named in the .urp too (`link -I` names a whole directory), or the
# package given its source as `src`.
{ lib }:
let
  # Directives of one word, which a .urp without a blank line could
  # otherwise be taken to list as modules.
  oneWord = [
    "debug"
    "dev"
    "profile"
    "html5"
    "xhtml"
    "noMangleSql"
    "lessSafeFfi"
    "safeGetDefault"
  ];

  # The lines of a .urp as the compiler sees them: a `#` starts a
  # comment to the end of the line, and a line that is only a comment is
  # not there.  A blank line stays, as "": it ends the directives.
  lines =
    urp:
    lib.concatMap (
      raw:
      let
        content = lib.trim (lib.head (lib.splitString "#" raw));
      in
      if content == "" && lib.hasInfix "#" raw then [ ] else [ content ]
    ) (lib.splitString "\n" (builtins.readFile urp));

  words = s: lib.filter lib.isString (builtins.split "[[:space:]]+" s);

  # A .urp has directives if some line is one; then they run to the
  # first blank line and the modules follow.  Otherwise every line is a
  # module.
  split =
    ls:
    let
      isDirective = l: lib.elem l oneWord || lib.match ".*[[:space:]].*" l != null;
    in
    if lib.any isDirective ls then
      let
        blank = lib.lists.findFirstIndex (l: l == "") (lib.length ls) ls;
      in
      {
        directives = map (l: lib.match "([^[:space:]]+)[[:space:]]*(.*)" l) (lib.take blank ls);
        modules = lib.filter (l: l != "") (lib.drop (blank + 1) ls);
      }
    else
      {
        directives = [ ];
        modules = lib.filter (l: l != "") ls;
      };

  # Adds what one .urp reads to acc, { files, pathmap, seen }.  As in the
  # compiler, there is one `path` map, which a `path` directive changes
  # for everything read after it, libraries included, and a library is
  # read once however often it is named.
  read =
    acc: urp:
    let
      dir = dirOf urp;

      # A path as written in this .urp, or null for one outside the tree.
      resolve =
        pathmap: p:
        if lib.hasPrefix "$" p then
          let
            m = lib.match "\\$([^/]*)(.*)" p;
            base = pathmap.${lib.elemAt m 0} or null;
          in
          if base == null then null else base + lib.elemAt m 1
        else if lib.hasPrefix "/" p then
          null
        else
          dir + "/${p}";

      # A library is NAME.urp, or NAME/lib.urp.
      library = p: if builtins.pathExists (p + ".urp") then p + ".urp" else p + "/lib.urp";

      # A module needs its .ur; the .urs is optional.
      module = p: [ (p + ".ur") ] ++ lib.optional (builtins.pathExists (p + ".urs")) (p + ".urs");

      step =
        acc: d:
        let
          cmd = lib.elemAt d 0;
          arg = lib.elemAt d 1;
          resolve' = resolve acc.pathmap;
          add = p: if p == null then acc else acc // { files = acc.files ++ [ p ]; };
        in
        if cmd == "library" then
          let
            p = resolve' arg;
          in
          if p == null || lib.elem (toString (library p)) acc.seen then acc else read acc (library p)
        else if cmd == "path" then
          let
            m = lib.match "([^=]*)=([^=]*)" arg;
            v = lib.elemAt m 1;
          in
          if m == null then
            acc
          else
            acc
            // {
              pathmap = acc.pathmap // {
                ${lib.elemAt m 0} = if lib.hasPrefix "/" v then null else dir + "/${v}";
              };
            }
        else if cmd == "file" then
          let
            ws = words arg;
          in
          if lib.length ws < 2 then acc else add (resolve' (lib.elemAt ws 1))
        else if cmd == "ffi" then
          let
            p = resolve' arg;
          in
          add (if p == null then null else p + ".urs")
        else if cmd == "link" then
          if lib.hasPrefix "-I" arg then
            let
              d = lib.trim (lib.removePrefix "-I" arg);
            in
            if d == "" then acc else add (resolve' d)
          else if lib.hasPrefix "-" arg then
            acc
          else
            add (resolve' arg)
        else if lib.elem cmd [
          "jsFile"
          "include"
          "mimeTypes"
        ] then
          add (resolve' arg)
        else
          acc;

      parsed = split (lines urp);

      directives = lib.foldl' step (
        acc
        // {
          files = acc.files ++ [ urp ];
          seen = acc.seen ++ [ (toString urp) ];
        }
      ) parsed.directives;

      modules = lib.concatMap (
        m:
        let
          p = resolve directives.pathmap m;
        in
        if p == null then [ ] else module p
      ) parsed.modules;
    in
    directives // { files = directives.files ++ modules; };

  files =
    urp:
    (read {
      files = [ ];
      pathmap = { };
      seen = [ ];
    } urp).files;
in
{
  # The project's .urp, a path, to the paths of everything it reads, itself first.
  inherit files;

  # The same as a fileset, for `lib.fileset.toSource`.
  fileset = urp: lib.fileset.unions (files urp);
}
