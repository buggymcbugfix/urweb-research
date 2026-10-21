# urweb.buildApp: an Ur/Web application as a package, built for FastCGI.
#
#   urweb.buildApp {
#     pname = "shop";
#     version = "1.0";
#     root = ./.;                     # the tree the .urp's paths are relative to
#     project = "ur/apps/shop/shop";  # the .urp, without the extension; pname by default
#     dbms = "sqlite";                # or "postgres"
#     prefix = "/";                   # where the application is served
#     urwebFlags = [ ];               # anything else for the compiler
#   }
#
# With root, the source is exactly what the .urp names, read as the
# compiler reads it (nix/project-files.nix): the modules, the libraries
# and the files served, included and linked.  Nothing else in the tree
# rebuilds the package.  src instead of root takes the sources as given.
#
# Any other attribute (postPatch, say) goes to mkDerivation.  The program
# is bin/<pname>, built with -protocol fastcgi, and the schema that
# creates its database is share/<pname>/schema.sql.
#
# The flags given here win over the .urp's own prefix, database and sql
# directives: the compiler reads those only when no flag set them.  So
# passthru.urweb.prefix is the prefix the program has.  A prefix goes
# here, not in the .urp.
#
# An application whose database is set up otherwise, and whose .urp
# may name a sql file only to keep track of what the code would create,
# says schema = false: the schema then goes to /dev/null, the .urp's sql
# is not written, and the package has no schema.
#
# The database is named when the program is started, by
# URWEB_SQLITE_DB_PATH or URWEB_PQ_CON.  What is compiled in cannot be
# opened, so a program started without the variable fails at once
# instead of opening some other database.
#
# passthru.urweb says how the program was built, for whatever deploys it:
# protocol, dbms, prefix, name (the pname), and schema, the path of the
# schema in the package, or null without one.
{ lib, stdenv, urweb }:
{
  pname,
  version ? "0",
  root ? null,
  src ? lib.fileset.toSource {
    inherit root;
    fileset = (import ./project-files.nix { inherit lib; }).fileset (root + "/${project}.urp");
  },
  project ? pname,
  dbms ? "sqlite",
  prefix ? "/",
  schema ? true,
  urwebFlags ? [ ],
  ...
}@args:
assert lib.assertMsg (args ? src || root != null) "urweb.buildApp: give root, the tree the .urp is in, or src.";
let
  database =
    if dbms == "sqlite" then "/nonexistent/URWEB_SQLITE_DB_PATH-is-not-set.db"
    else "host=/nonexistent/URWEB_PQ_CON-is-not-set";
  schemaPath = "share/${pname}/schema.sql";
in
stdenv.mkDerivation (finalAttrs:
  removeAttrs args [ "root" "project" "dbms" "prefix" "schema" "urwebFlags" ]
  // {
    inherit pname version src;
    nativeBuildInputs = [ urweb ] ++ (args.nativeBuildInputs or [ ]);

    buildPhase = ''
      runHook preBuild
      ${lib.getExe urweb} \
        -protocol fastcgi \
        -dbms ${dbms} \
        -db ${lib.escapeShellArg database} \
        -prefix ${lib.escapeShellArg prefix} \
        -sql ${if schema then "schema.sql" else "/dev/null"} \
        ${lib.escapeShellArgs urwebFlags} \
        -output ${pname}.exe \
        ${project}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      install -D ${pname}.exe $out/bin/${pname}
      ${lib.optionalString schema "install -D -m 644 schema.sql $out/${schemaPath}"}
      runHook postInstall
    '';

    passthru = (args.passthru or { }) // {
      urweb = {
        protocol = "fastcgi";
        inherit dbms prefix;
        name = pname;
        schema = if schema then "${finalAttrs.finalPackage}/${schemaPath}" else null;
      };
    };

    meta = (args.meta or { }) // {
      mainProgram = pname;
    };
  }
)
