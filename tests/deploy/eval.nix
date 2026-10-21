# What urweb.buildApp and nix/project-files.nix make of their arguments,
# checked without a virtual machine against the files under expected/:
#
#   nix-build tests/deploy/eval.nix    # fails with the diff when they differ
#   tests/deploy/update.sh             # rewrites expected/ from what they make now
#
# expected/build-app: the build and install phases buildApp writes, and
#   passthru.urweb, for several arguments.
# expected/project-files: the files project-files.nix finds for .urp
#   files of this tree.
#
# Store hashes are masked, so a change of the compiler or of a package
# shows only where a version or what buildApp writes changes.
{ hostPkgs ? import ../../nixpkgs.nix }:
let
  # Linux's packages whatever the machine: the applications are for Linux.
  pkgs = hostPkgs.pkgsLinux;
  inherit (pkgs) lib;

  urweb = pkgs.callPackage ../../derivation.nix { };
  app = args: pkgs.callPackage ./app.nix ({ inherit urweb; } // args);

  mask = s: lib.concatMapStrings (x: if lib.isList x then "/nix/store/HASH-" else x)
    (builtins.split "/nix/store/[a-z0-9]{32}-" (builtins.unsafeDiscardStringContext s));

  buildApps = {
    sqlite = app { };
    postgres = app { dbms = "postgres"; };
    no-schema = app { schema = false; };
    prefix = app { prefix = "/shop/"; };
  };
  renderBuild = p: mask ''
    ### buildPhase
    ${p.buildPhase}
    ### installPhase
    ${p.installPhase}
    ### passthru.urweb
    ${builtins.toJSON p.urweb}
  '';

  # .urp files of this tree with directives that name files.
  urps = [
    "demo/more/grid1"
    "tests/bench/app/bench"
    "tests/deploy/app/app"
    "tests/ffieff"
    "tests/files"
    "tests/include_header/include_header"
    "tests/library2"
    "tests/link_c/link_c"
    "tests/link_c_dirs/link_c_dirs"
    "tests/link_c_flags/link_c_flags"
    "tests/mimeTypesDirective"
    "tests/multilib"
    "tests/pathmap"
    "tests/polyjs"
    "tests/static_file/static_file"
  ];
  projectFiles =
    let
      pf = import ../../nix/project-files.nix { inherit lib; };
      rel = f: lib.removePrefix (toString ../.. + "/") (toString f);
    in
    lib.concatMapStrings (u: "### ${u}.urp\n" + lib.concatMapStrings (f: rel f + "\n") (pf.files (../.. + "/${u}.urp"))) urps;

  rendered = pkgs.runCommand "urweb-deploy-eval-rendered" { } ''
    mkdir -p $out/build-app
    ${lib.concatStrings (lib.mapAttrsToList (n: p: ''
      cp ${pkgs.writeText "build-app-${n}" (renderBuild p)} $out/build-app/${n}
    '') buildApps)}
    cp ${pkgs.writeText "project-files" projectFiles} $out/project-files
  '';
in
pkgs.runCommand "urweb-deploy-eval" { passthru = { inherit rendered; }; } ''
  diff -ru ${./expected} ${rendered}
  touch $out
''
