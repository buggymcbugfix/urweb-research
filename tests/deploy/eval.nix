# What nixos/urweb.nix, urweb.buildApp and nix/project-files.nix make of
# their arguments, checked without a virtual machine against the files
# under expected/:
#
#   nix-build tests/deploy/eval.nix    # fails with the diff when they differ
#   tests/deploy/update.sh             # rewrites expected/ from what they make now
#
# expected/units/<case>: the units of a configuration, the preStart,
#   <name>-db, the users, nginx's upstreams and locations, the
#   module's tmpfiles rules, and its failed assertions.
# expected/prestart: the preStart of a SQLite application, run here on
#   a fresh directory and on what can go wrong there.
# expected/build-app: the build and install phases buildApp writes, and
#   passthru.urweb, for several arguments.
# expected/project-files: the files project-files.nix finds for .urp
#   files of this tree.
# expected/references: what a package buildApp builds references, and
#   what its program loads at run time. The one record that builds
#   the compiler and an application; the rest is evaluation.
#
# expected/vm is not among them: it is the record of the virtual
# machine, which test.nix compares.
#
# Store hashes are masked, so a change of the compiler or of a package
# shows only where a version or what the module writes changes.
{ hostPkgs ? import ../../nixpkgs.nix }:
let
  # Linux's packages whatever the machine, as the virtual machine's are:
  # the units, and the scripts run here, are for Linux.
  pkgs = hostPkgs.pkgsLinux;
  inherit (pkgs) lib;

  urweb = pkgs.callPackage ../../derivation.nix { };
  app = args: pkgs.callPackage ./app.nix ({ inherit urweb; } // args);

  mask = s: lib.concatMapStrings (x: if lib.isList x then "/nix/store/HASH-" else x)
    (builtins.split "/nix/store/[a-z0-9]{32}-" (builtins.unsafeDiscardStringContext s));

  # A NixOS configuration with the module and the given apps.
  eval = apps: (import (pkgs.path + "/nixos/lib/eval-config.nix") {
    inherit pkgs;
    inherit (pkgs.stdenv.hostPlatform) system;
    modules = [
      ../../nixos/urweb.nix
      {
        boot.loader.grub.enable = false;
        fileSystems."/" = { device = "none"; fsType = "tmpfs"; };
        system.stateVersion = lib.trivial.release;
        services.urweb.apps = apps;
      }
    ];
  }).config;

  # A package that says it was built otherwise than buildApp builds.
  other = passthru: pkgs.runCommand "other" {
    passthru.urweb = { name = "other"; } // passthru;
    meta.mainProgram = "other";
  } "";

  render = config:
    let
      apps = lib.attrNames config.services.urweb.apps;
      cfg = config.services.urweb.apps;
      section = title: body: "### ${title}\n${body}";
      db = n: lib.findFirst (p: (p.name or "") == "${n}-db") null config.environment.systemPackages;
      hosts = lib.unique (lib.filter (h: h != null) (map (n: cfg.${n}.nginx.virtualHost) apps));
      upstream = u: "servers ${toString (lib.attrNames u.servers)}\n${u.extraConfig}\n";
      location = l: v: "location ${l} {\n${v.extraConfig}}\n";
      failed = lib.filter (a: !a.assertion && lib.hasPrefix "services.urweb" a.message) config.assertions;
      # The module's rules, each as a line of tmpfiles.d(5).
      tmpfiles = config.systemd.tmpfiles.settings.urweb or { };
      rules = path: entries: lib.concatStrings (lib.mapAttrsToList (_: e:
        lib.concatStringsSep " " ([ e.type path e.mode e.user e.group e.age ] ++ lib.optional (e.argument != "") e.argument) + "\n") entries);
    in
    mask (lib.concatStringsSep "\n" (
      lib.concatMap (n: let u = "urweb-${n}"; in [
        (section "${u}.socket" config.systemd.units."${u}.socket".text)
        (section "${u}.service" config.systemd.units."${u}.service".text)
        (section "${u}.service preStart" config.systemd.services.${u}.preStart)
        (section "${n}-db" (if db n == null then "(none)\n" else (db n).text))
        (section "user ${u}" "group ${config.users.users.${u}.group}\n")
        (section "nginx upstream ${u}" (upstream config.services.nginx.upstreams.${u}))
      ]) apps
      ++ map (h: section "nginx server ${h}"
        (lib.concatStrings (lib.mapAttrsToList location config.services.nginx.virtualHosts.${h}.locations))) hosts
      ++ [
        (section "tmpfiles" (if tmpfiles == { } then "(none)\n" else lib.concatStrings (lib.mapAttrsToList rules tmpfiles)))
        (section "failed assertions" (lib.concatMapStrings (a: a.message + "\n") failed))
      ]
    ));

  cases = {
    # The virtual machine's configuration.
    deploy = (import ./test.nix { pkgs = hostPkgs; }).vm.nodes.machine;
    # Two applications on one virtual host, under their prefixes.
    same-host = eval {
      shop = { package = app { prefix = "/shop/"; }; nginx.virtualHost = "example.org"; };
      erp = { package = app { prefix = "/erp/"; }; nginx.virtualHost = "example.org"; };
    };
    postgres = eval {
      pg = {
        package = app { dbms = "postgres"; };
        environment.URWEB_PQ_CON = "dbname=urweb-pg";
        nginx.virtualHost = "example.org";
      };
    };
    # A database set up otherwise, and a program that keeps no idle
    # connections.
    no-schema = eval {
      app = { package = app { schema = false; }; nginx.keepalive = 0; };
    };
    # A path with a space.
    space = eval {
      app = { package = app { }; database.path = "/var/lib/my db/app.db"; };
    };
    refused = eval {
      http = { package = other { protocol = "http"; }; };
      few = { package = app { }; threads = 16; nginx.keepalive = 8; };
      # Two applications at one place.
      here = { package = app { }; nginx.virtualHost = "example.org"; };
      there = { package = app { }; nginx.virtualHost = "example.org"; };
      # A database the program is told about otherwise than by database.path.
      elsewhere = { package = app { }; environment.URWEB_SQLITE_DB_PATH = "/srv/other.db"; };
      # A relative database path.
      relative = { package = app { }; database.path = "db/app.db"; };
    };
  };

  # The preStart of each scenario, run on directories under $DB, which
  # stands for /@DB@ in the paths it was evaluated with.
  scenarios = eval (lib.genAttrs [ "fresh" "bad" "empty" "missing" ] (n: {
    package = app { };
    schema = if n == "bad" then ./prestart/bad.sql else ./prestart/schema.sql;
    database.path = if n == "missing" then "/@DB@/missing/sub/missing.db" else "/@DB@/${n}/${n}.db";
  }) // {
    space = {
      package = app { };
      schema = ./prestart/schema.sql;
      database.path = "/@DB@/space dir/space.db";
    };
  });
  preStart = n: pkgs.writeText "${n}-pre-start" scenarios.systemd.services."urweb-${n}".preStart;
  dbScript = n: lib.getExe' (lib.findFirst (p: (p.name or "") == "${n}-db") null scenarios.environment.systemPackages) "${n}-db";

  prestart = pkgs.runCommand "urweb-deploy-prestart" { nativeBuildInputs = [ pkgs.sqlite ]; } ''
    DB=$PWD/db
    mkdir -p "$DB"/fresh "$DB"/bad "$DB"/empty "$DB"/missing "$DB/space dir"

    # localize FILE: a copy of FILE with /@DB@ replaced by $DB, and so every script it names.
    localize() {
      local out
      out=$(mktemp -p "$PWD")
      cp "$1" "$out"
      chmod u+w "$out"
      for s in $(grep -o '/nix/store/[a-z0-9]*-[^ "'"'"']*' "$1" | sort -u); do
        if [ -f "$s" ] && grep -q '/@DB@' "$s"; then
          sed -i "s|$s|$(localize "$s")|g" "$out"
        fi
      done
      sed -i "s|/@DB@|$DB|g" "$out"
      chmod +x "$out"
      echo "$out"
    }
    # start NAME PRESTART: runs the preStart of NAME and shows what it left.
    start() {
      bash "$(localize "$2")" 2>&1
      echo "exit $?"
      (cd "$DB" && find . -path "./$1*" -printf '%p %y %m\n' | sort)
      for f in "$DB/$1"*/*.db; do
        if [ -f "$f" ]; then echo "tables:" $(sqlite3 "$f" .tables); fi
      done
    }
    set +e +o pipefail # the scenarios fail on purpose
    {
      umask 0007 # the service's
      echo "### fresh"; start fresh ${preStart "fresh"}
      echo "### fresh, started again"; start fresh ${preStart "fresh"}
      echo "### bad: a schema that fails half-way"; start bad ${preStart "bad"}
      echo "### bad, started again"; start bad ${preStart "bad"}
      echo "### empty: empty-db run before the first start"
      "$(localize ${dbScript "empty"})" 'select count(*) from t' 2>&1; echo "exit $?"
      start empty ${preStart "empty"}
      echo "### missing: a database in a directory that does not exist"; start missing ${preStart "missing"}
      echo "### space: a database in a directory with a space"; start space ${preStart "space"}
    } | sed "s|$DB|/DB|g; s|$PWD/tmp\.[A-Za-z0-9]*|/BUILD/TMP|g; s|/nix/store/[a-z0-9]\{32\}-|/nix/store/HASH-|g" > $out
  '';

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

  # What the package of an application references, the shared libraries
  # its program needs and the directories it loads them from (the
  # RUNPATH the linker wrote, in its order).  A package among the
  # references is in every deployment of the application, with its own
  # closure.  Versions are dropped along with the hashes, so that a bump
  # of nixpkgs leaves the record alone: a segment of a store name that
  # starts with a digit (glibc-2.42-67 is glibc, urweb-20200209-lib is
  # urweb-lib), and a shared library's suffix (libssl.so.3 is libssl).
  linked = {
    sqlite = app { };
    postgres = app { dbms = "postgres"; };
  };
  references = pkgs.runCommand "urweb-deploy-references" { } (
    ''
      unversion() {
        sed -E 's|/nix/store/[a-z0-9]{32}-|/nix/store/HASH-|g; :a; s|(/nix/store/HASH-[^/ ]*)-[0-9][^-/ ]*|\1|; ta'
      }
      {
    ''
    + lib.concatStrings (lib.mapAttrsToList (n: p: ''
      echo "### ${n}"
      echo "references:"
      unversion < ${pkgs.writeDirectReferencesToFile p} | sed 's|^/nix/store/HASH-||' | sort
      echo "needed:"
      patchelf --print-needed ${lib.getExe p} | sed -E 's/\.so(\.[0-9.]+)?$//'
      echo "runpath:"
      patchelf --print-rpath ${lib.getExe p} | tr : '\n' | unversion
    '') linked)
    + ''
      } > $out
    ''
  );

  rendered = pkgs.runCommand "urweb-deploy-eval-rendered" { } ''
    mkdir -p $out/units $out/build-app
    ${lib.concatStrings (lib.mapAttrsToList (n: c: ''
      cp ${pkgs.writeText "unit-${n}" (render c)} $out/units/${n}
    '') cases)}
    ${lib.concatStrings (lib.mapAttrsToList (n: p: ''
      cp ${pkgs.writeText "build-app-${n}" (renderBuild p)} $out/build-app/${n}
    '') buildApps)}
    cp ${pkgs.writeText "project-files" projectFiles} $out/project-files
    cp ${prestart} $out/prestart
    cp ${references} $out/references
  '';
in
pkgs.runCommand "urweb-deploy-eval" { passthru = { inherit rendered; }; } ''
  diff -ru -x vm ${./expected} ${rendered}
  touch $out
''
