{
  autoconf,
  automake,
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
  # The commit `urweb -version` names, `<hash>` or `<hash>-dirty`.
  # Read from .git unless given. It can be set here for flakes.
  rev ? null,
  # Build `urt` too? `nix-build --arg withUrt true` / `urweb.override { withUrt = true; }`
  withUrt ? false,
}:

let
  commit =
    if rev != null then
      rev
    else if lib.path.hasStorePathPrefix ./. then
      "?"
    else
      let
        git = builtins.fetchGit {
          url = ./.;
          shallow = true;
        };
      in
      git.dirtyRev or git.rev;

  configureEnv = prefix: ''
    export SQHEADER="${sqlite.dev}/include/sqlite3.h"
    export PGHEADER="${postgresql.dev}/include/libpq-fe.h"
    export ICU_INCLUDES="-I${icu.dev}/include"
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
in

stdenv.mkDerivation (finalAttrs: {
  pname = "urweb";
  version = "20200209";

  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.intersection (lib.fileset.gitTracked ./.) (
      lib.fileset.unions (
        [
          ./autogen.sh
          ./configure.ac
          ./demo
          ./doc
          ./include
          ./lib
          ./m4
          ./Makefile.am
          ./src
          ./tests
          ./xml
        ]
        ++ lib.optional withUrt ./urt
      )
    );
  };

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

  # test dependencies
  nativeCheckInputs = [
    curl # the demo test's requests, and libcurl for tests/io_curl
    spawn_fcgi # which starts that program on its socket
    python3 # tests/io_curl's stub server; urt's pty cases and stand-in server
  ];

  # What the build writes into src/version.sml (see Makefile.am): the
  # sandbox has neither git nor .git to read it from
  env.URWEB_COMMIT = commit;

  preConfigure = configureEnv "$out" + ''
    ./autogen.sh
  '';

  # The urweb compiler links generated applications against the static
  # archives, so keep the .a files in the output.
  dontDisableStatic = true;

  buildPhase = ''
    runHook preBuild
    make ${lib.optionalString withUrt "all urt"}
    runHook postBuild
  '';

  doCheck = true;

  checkPhase = ''
    runHook preCheck
    make check ${lib.optionalString withUrt "check-urt check-snapshot"}
    runHook postCheck
  '';

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
    in
    runCommand "urweb-with-libs"
      {
        nativeBuildInputs = [ makeBinaryWrapper ];
        meta.mainProgram = "urweb";
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
