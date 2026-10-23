{
  autoconf,
  automake,
  callPackage,
  curl,
  gcc,
  icu,
  lib,
  libtool,
  linkFarm,
  makeBinaryWrapper,
  mlton20241230,
  openssl,
  pkg-config,
  postgresql,
  python3,
  runCommand,
  spawn_fcgi,
  sqlite,
  stdenv,
  # The commit `urweb -version` names, for a release: a flake's self.rev,
  # say. Without it the version names none, and the package depends on its
  # sources alone, not on the commit they were taken from.
  rev ? null,
  # Build `urt` too? `nix-build --arg withUrt true` / `urweb.override { withUrt = true; }`
  withUrt ? false,
}:

let
  # What configure is told: the headers the compiler is built with, and
  # the directories of the shared libraries an application links, ICU,
  # OpenSSL, SQLite and libpq.  A package that builds an application has
  # the compiler among its inputs and none of these, so the compiler
  # carries their directories.
  configureEnv = prefix: ''
    export SQHEADER="${sqlite.dev}/include/sqlite3.h"
    export PGHEADER="${postgresql.dev}/include/libpq-fe.h"
    export ICU_INCLUDES="-I${icu.dev}/include"
    export ICU_LIBS="-L${lib.getLib icu}/lib"
    export CC="${gcc}/bin/gcc"
    export CCARGS="-I${prefix}/include \
      -L${lib.getLib openssl}/lib \
      -L${sqlite.out}/lib \
      -L${postgresql.lib}/lib \
      -Wno-error=int-conversion"
  '';

  # MLton compiles the SML/NJ Library into every program, from the sources it
  # installs under lib/mlton/sml. MLton 20241230 has SML/NJ 110.99.6.1 there,
  # whose IntBinarySet.split_gt has its two comparisons the wrong way round.
  # union, intersection and difference then return wrong sets, and the
  # compiler hangs in core_untangle or fails later on. SML/NJ 110.99.9 has
  # the fix (smlnj/smlnj 9cdd824, by Darin Minamoto). The mlton script finds
  # its library relative to itself, so a copy of MLton with that file fixed
  # needs no rebuild. --replace-fail stops the build once MLton has the fix.
  mlton = runCommand "${mlton20241230.name}-intbinaryset-fix" { } ''
    cp -r ${mlton20241230} $out
    chmod -R u+w $out
    substituteInPlace $out/lib/mlton/sml/smlnj-lib/Util/int-binary-set.sml \
      --replace-fail 'if (x < v) then split_gt(r,x)' 'if (x > v) then split_gt(r,x)' \
      --replace-fail 'else if (x > v) then concat3(split_gt(l,x),v,r)' \
        'else if (x < v) then concat3(split_gt(l,x),v,r)'
  '';

  # spawn-fcgi starts the FastCGI programs of the tests and of tests/bench
  # on their Unix sockets. nixpkgs' 1.6.6 binds the socket under a name one
  # byte short on the BSDs and macOS, where a feature macro it defines
  # hides SUN_LEN (see the patch), and the program never comes up. The
  # patch computes the length the way SUN_LEN does, on every platform, so
  # the Linux builds exercise it too. Drop it once the pinned nixpkgs has
  # a spawn-fcgi with the fix.
  spawnFcgi = spawn_fcgi.overrideAttrs (old: {
    patches = (old.patches or [ ]) ++ [ ./nix/patches/spawn-fcgi/sun-len.patch ];
  });

  # The source is the tracked files in a fileset.
  fromGit =
    fileset:
    lib.fileset.toSource {
      root = ./.;
      fileset = lib.fileset.intersection (lib.fileset.gitTracked ./.) fileset;
    };

  # What the package reads, and nothing else: a change to anything else
  # would build the compiler again for nothing. The tests and the demo are
  # not among them. The checks read them (see passthru.tests.check), so a
  # change to a test runs the checks again and leaves the package alone.
  buildFiles = lib.fileset.unions (
    [
      ./autogen.sh
      ./configure.ac
      ./include
      ./lib
      ./m4
      ./Makefile.am
      ./src
      ./xml
    ]
    ++ lib.optionals withUrt [
      ./urt/Makefile
      ./urt/src
    ]
  );

  # What the checks read besides.
  testFiles = lib.fileset.difference (lib.fileset.unions (
    [
      ./demo
      ./tests
    ]
    ++ lib.optional withUrt ./urt
  )) (
    lib.fileset.unions [
      # Runs nix itself, outside the build (make check-shell-id).
      ./tests/shell_id
      # The NixOS module's tests, which use the package.
      ./tests/deploy
      # The benchmark, apart from its application, whose snapshots
      # check-snapshot runs.
      (lib.fileset.difference ./tests/bench ./tests/bench/app)
    ]
  );
in

stdenv.mkDerivation (finalAttrs: {
  pname = "urweb";
  version = "20200209";

  src = fromGit buildFiles;

  # The runtime's libraries go to an output of their own: the compiler
  # links an application against the archives by default, and with
  # -dynamic against the shared libraries, which the program then loads
  # from here.  With the libraries beside the compiler, such a program
  # would carry the compiler and its toolchains in its closure.  The
  # compiler finds them through configure's --libdir, which the output
  # sets.  The library sources under lib/urweb and the headers stay with
  # the compiler.
  outputs = [
    "out"
    "lib"
  ];

  # A program built with -dynamic carries the lib output and whatever is
  # behind it, so nothing of the compiler's may be: not the compiler, which
  # Nix sees to on its own (the compiler references the output, and a
  # reference back would be a cycle between outputs, refused), and not
  # gcc or the dev outputs the compiler references, which a libtool .la
  # file could name.  outputChecks needs structured attributes.
  __structuredAttrs = true;
  outputChecks.lib.disallowedRequisites = [
    gcc
    postgresql.dev
    sqlite.dev
    icu.dev
  ];

  # build-time dependencies
  nativeBuildInputs = [
    autoconf
    automake
    libtool
    mlton
    pkg-config
  ]
  ++ lib.optional withUrt makeBinaryWrapper;

  # link/runtime dependencies
  buildInputs = [
    icu
    openssl
    postgresql
    sqlite
  ];

  # What the build writes into src/version.sml (see Makefile.am).
  env.URWEB_COMMIT = if rev == null then "?" else rev;

  preConfigure = configureEnv "$out" + ''
    ./autogen.sh
  '';

  # nixpkgs configures with --disable-static, and the compiler links
  # applications against the archives by default.
  dontDisableStatic = true;

  buildPhase = ''
    runHook preBuild
    make ${lib.optionalString withUrt "all urt"}
    runHook postBuild
  '';

  # The checks are passthru.tests.check, a derivation of their own.
  doCheck = false;

  installPhase = ''
    runHook preInstall
    make install
  ''
  + lib.optionalString withUrt ''
    install -m 755 urt/urt urt/urt-format "$out/bin"
    wrapProgram "$out/bin/urt" --set-default URWEB "$out/bin/urweb"
  ''
  + ''
    runHook postInstall
  '';

  passthru.configureEnv = configureEnv;

  # The NixOS module that runs a FastCGI program as a service behind nginx:
  # imports = [ urweb.nixosModule ]; see the file.
  passthru.nixosModule = ./nixos/urweb.nix;

  # urweb.buildApp { pname, src, ... }: an application as a package, built
  # as the module deploys it; see nix/build-app.nix.
  passthru.buildApp = callPackage ./nix/build-app.nix { urweb = finalAttrs.finalPackage; };

  /*
    The checks, run on the compiler this package installs, not on one built
    for them: make installcheck in a tree configured with the package's
    prefix (see installcheck-local in Makefile.am). The tests, the demo and
    (with urt) urt's own checks are inputs of this derivation and not of the
    package, so a change to them runs the checks again and leaves the
    compiler as it is. Configuring takes a few seconds and builds nothing.

    nix-build -A check (default.nix), or urweb.tests.check
  */
  passthru.tests.check = stdenv.mkDerivation {
    pname = "urweb-check";
    inherit (finalAttrs) version;

    src = fromGit (lib.fileset.union buildFiles testFiles);

    nativeBuildInputs = finalAttrs.nativeBuildInputs ++ [
      curl # the demo test's requests, and libcurl for tests/io_curl
      spawnFcgi # which starts that program on its socket
      python3 # tests/io_curl's stub server and tests/fastcgi_idle's client; urt's pty cases and stand-in server
    ];

    # The applications the tests compile link against these.
    inherit (finalAttrs) buildInputs;

    # configure --prefix: where make installcheck finds the compiler.
    prefix = finalAttrs.finalPackage;
    preConfigure = configureEnv "${finalAttrs.finalPackage}" + ''
      ./autogen.sh
    '';

    dontBuild = true;

    # Nothing to install: the output only records that the checks passed.
    installPhase = ''
      touch $out
    '';

    dontFixup = true;

    doInstallCheck = true;

    installCheckPhase = ''
      runHook preInstallCheck
      make installcheck ${lib.optionalString withUrt "INSTALLCHECKS='check-local check-urt check-snapshot'"}
      runHook postInstallCheck
    '';
  };

  /*
    withLibraries accepts urweb libraries:

      urweb-with-libs = urweb.withLibraries {
        foo = someLib;
      };

    Use it by calling the default executable:

      ${lib.getExe urweb-with-libs} ...

    This will allow importing the libraries in .urp files using

      library $NIX_LIBS/foo
  */
  passthru.withLibraries =
    libs:
    let
      libPath = linkFarm "urweb-libs" (lib.mapAttrsToList (name: path: { inherit name path; }) libs);
      urweb = finalAttrs.finalPackage;
      withLibs = runCommand "urweb-with-libs"
        {
          nativeBuildInputs = [ makeBinaryWrapper ];
          meta.mainProgram = "urweb";
          # buildApp with this compiler, so that the application imports the libraries.
          passthru.buildApp = callPackage ./nix/build-app.nix { urweb = withLibs; };
        }
        (
          ''
            makeWrapper ${lib.getExe urweb} $out/bin/urweb \
              --add-flags "-path NIX_LIBS ${libPath}"
          ''
          + lib.optionalString withUrt ''
            makeWrapper ${urweb}/bin/urt $out/bin/urt --set-default URWEB $out/bin/urweb
          ''
        );
    in
    withLibs;

  meta = {
    description = "Advanced purely-functional web programming language";
    mainProgram = "urweb";
    homepage = "http://www.impredicative.com/ur/";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux ++ lib.platforms.darwin;
    maintainers = [
      lib.maintainers.buggymcbugfix
    ];
  };
})
