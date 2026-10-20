# urweb.buildApp: an Ur/Web application as a package, built the way
# nixos/urweb.nix deploys it.
#
#   urweb.buildApp {
#     pname = "shop";
#     version = "1.0";
#     root = ./.;                     # the tree the .urp's paths are relative to
#     project = "ur/apps/shop/shop";  # the .urp, without the extension; pname by default
#     dbms = "sqlite";                # or "postgres"
#     prefix = "/";                   # where the application is served; the module's nginx location follows it
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
# The flags given here win over the .urp's own database and sql
# directives: the compiler reads those only when no flag set them.  The
# exception is the prefix "/", which the compiler takes for no flag at
# all, so that a .urp's own prefix applies and passthru.urweb.prefix,
# which the module's nginx location follows, does not say it.  A prefix
# goes here, not in the .urp.
#
# An application whose database is set up otherwise, and whose .urp
# names a sql file only to keep track of what the code would create,
# says schema = false: no -sql then, so the .urp's sql is written where
# it says, and no schema in the package.  The directory it names is
# made before the build, since the source need not carry it.
#
# The database is named when the program is started, by
# URWEB_SQLITE_DB_PATH or URWEB_PQ_CON, as the module does.  What is
# compiled in cannot be opened, so a program started without the variable
# fails at once instead of opening some other database.
#
# passthru.urweb says how the program was built, for the module:
# protocol, dbms, prefix, schema, and name, the pname.
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
    else "host=/nonexistent dbname=URWEB_PQ_CON-is-not-set";
in
stdenv.mkDerivation (
  removeAttrs args [ "root" "project" "dbms" "prefix" "schema" "urwebFlags" ]
  // {
    inherit pname version src;
    nativeBuildInputs = [ urweb ] ++ (args.nativeBuildInputs or [ ]);

    buildPhase = ''
      runHook preBuild
      # Without -sql, the schema goes where the .urp says, relative to the .urp; the directory has to exist.
      sql=$(sed -n 's/#.*//; s/^sql[[:space:]]\{1,\}//p' ${project}.urp | head -n 1 | sed 's/[[:space:]]*$//')
      case "$sql" in
        "" | /*) ;;
        *) mkdir -p "$(dirname "$(dirname ${project}.urp)/$sql")" ;;
      esac
      ${lib.getExe urweb} \
        -protocol fastcgi \
        -dbms ${dbms} \
        -db ${lib.escapeShellArg database} \
        -prefix ${lib.escapeShellArg prefix} \
        ${lib.optionalString schema "-sql schema.sql"} \
        ${lib.escapeShellArgs urwebFlags} \
        -output ${pname}.exe \
        ${project}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      install -D ${pname}.exe $out/bin/${pname}
      ${lib.optionalString schema "install -D -m 644 schema.sql $out/share/${pname}/schema.sql"}
      runHook postInstall
    '';

    passthru = (args.passthru or { }) // {
      urweb = {
        protocol = "fastcgi";
        inherit dbms prefix schema;
        name = pname;
      };
    };

    meta = (args.meta or { }) // {
      mainProgram = pname;
    };
  }
)
