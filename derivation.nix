{
  autoconf,
  automake,
  curl,
  dejavu_fonts,
  fetchzip,
  fontconfig,
  liberation_ttf,
  gcc,
  icu,
  lib,
  libglvnd,
  libtool,
  linkFarm,
  makeBinaryWrapper,
  makeFontsConf,
  mesa,
  mlton20241230,
  openssl,
  pkg-config,
  playwright-driver,
  postgresql,
  python3,
  runCommand,
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

  # urt's browser runs drive the three engines through Playwright for
  # Python; all of it comes with urt for now.  The browsers are nixpkgs'
  # own farm of them, with the ffmpeg Playwright records its videos
  # through (`urt snapshot --record`), and with one change: Playwright's
  # WebKit is WPE, which draws through EGL and finds none where there is
  # no GPU driver -- a shell, a build sandbox -- so it is given Mesa's
  # software one.
  urtPython = python3.withPackages (p: [ p.playwright ]);
  pw = playwright-driver.passthru;
  webkitWithEgl =
    if stdenv.hostPlatform.isLinux then
      runCommand "playwright-webkit-egl" { nativeBuildInputs = [ makeBinaryWrapper ]; } ''
        mkdir -p $out
        for f in ${pw.components.webkit}/*; do ln -s "$f" $out/; done
        rm $out/pw_run.sh
        makeWrapper ${pw.components.webkit}/pw_run.sh $out/pw_run.sh \
          --prefix LD_LIBRARY_PATH : ${lib.makeLibraryPath [ libglvnd mesa ]} \
          --set __EGL_VENDOR_LIBRARY_DIRS ${mesa}/share/glvnd/egl_vendor.d \
          --set EGL_PLATFORM surfaceless \
          --set LIBGL_ALWAYS_SOFTWARE 1
      ''
    else
      pw.components.webkit;
  # On a Mac, Playwright's WebKit is built for one macOS: the revision
  # above is the build for macOS 15, which nixpkgs fetches, and for macOS
  # 14 Playwright asks for an older one by its revision (browsers.json,
  # revisionOverrides), under a directory named for the platform as well
  # (webkit_mac14_arm64_special-2251), so the farm holds that one too,
  # and the host's macOS says which is started.  Hash of the 2251 build
  # for 1.61.1.
  webkitMac14Revision = pw.browsersJSON.webkit.revisionOverrides."mac14-arm64" or null;
  webkitMac14 = fetchzip {
    url = "https://cdn.playwright.dev/dbazure/download/playwright/builds/webkit/${webkitMac14Revision}/webkit-mac-14-arm64.zip";
    stripRoot = false;
    hash = "sha256-AbDHuUg8jLNPWur6hieDdY8Kc2+PmlXRJGD46yujam4=";
  };
  # Named as Playwright looks them up: chromium-1228, chromium_headless_shell-1228, ...
  farmName = name: "${lib.replaceStrings [ "-" ] [ "_" ] name}-${pw.browsersJSON.${name}.revision}";
  urtBrowsers = linkFarm "urt-browsers" (
    lib.listToAttrs (
      map (name: lib.nameValuePair (farmName name) pw.components.${name}) [
        "chromium"
        "chromium-headless-shell"
        "firefox"
        "ffmpeg"
      ]
    )
    // {
      ${farmName "webkit"} = webkitWithEgl;
    }
    // lib.optionalAttrs (stdenv.hostPlatform.isDarwin && webkitMac14Revision != null) {
      "webkit_mac14_arm64_special-${webkitMac14Revision}" = webkitMac14;
    }
  );
  # Where Playwright finds them; that it need not check the host for their
  # libraries, which were patched for the store when built (the check, ldd
  # over every library, takes seconds at every start); and the fonts the
  # browser draws with, so that a page is laid out the same in the shell,
  # in the package's checks and on another machine, rather than with
  # whatever the host has installed, which in a build sandbox is nothing.
  # Liberation and DejaVu, and one rule of our own, so that the engines
  # draw the same faces.  Chromium asks for its defaults by name, Times
  # New Roman, Arial and Courier New, and takes only a metric equivalent
  # for them, which Liberation is; Firefox asks for the generic serif,
  # sans-serif and monospace, which fontconfig's own rules would give to
  # DejaVu.  The rule gives the generics to Liberation too.  WebKit's
  # default is sans-serif where the other two default to serif, which is
  # the engine's and stays.
  fontRules = builtins.toFile "urt-fonts.conf" ''
    <?xml version="1.0"?>
    <!DOCTYPE fontconfig SYSTEM "fonts.dtd">
    <fontconfig>
      <alias binding="strong"><family>serif</family><prefer><family>Liberation Serif</family></prefer></alias>
      <alias binding="strong"><family>sans-serif</family><prefer><family>Liberation Sans</family></prefer></alias>
      <alias binding="strong"><family>monospace</family><prefer><family>Liberation Mono</family></prefer></alias>
    </fontconfig>
  '';
  fontsConf = makeFontsConf {
    fontDirectories = [
      dejavu_fonts
      liberation_ttf
    ];
    impureFontDirectories = [ ]; # nothing from the host, and its rules neither:
    includes = [
      "${fontconfig.out}/etc/fonts/conf.d" # fontconfig's own
      fontRules
    ];
  };
  browserEnv = {
    PLAYWRIGHT_BROWSERS_PATH = "${urtBrowsers}";
    PLAYWRIGHT_SKIP_VALIDATE_HOST_REQUIREMENTS = "1";
    # Less the user's own font directory, which fontconfig's template puts in
    FONTCONFIG_FILE = runCommand "urt-fonts.conf" { } ''
      sed '/<dir prefix="xdg">/d' ${fontsConf} > $out
    '';
  };

  # The same, as wrapProgram takes it
  browserFlags = lib.concatStringsSep " " (
    lib.mapAttrsToList (name: value: "--set-default ${name} ${lib.escapeShellArg value}") browserEnv
  );

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
  ++ lib.optionals withUrt [
    makeBinaryWrapper
    urtPython # urt's pty cases, stand-in server and browser runs; also what urt-browse's shebang becomes
  ];

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
    python3 # tests/io_curl's stub server
  ];

  # What the build writes into src/version.sml (see Makefile.am): the
  # sandbox has neither git nor .git to read it from; and, with urt,
  # where its checks find the browsers
  env = {
    URWEB_COMMIT = commit;
  }
  // lib.optionalAttrs withUrt browserEnv;

  # The build's sandbox has no /usr/bin/env for urt-browse's shebang, and
  # the checks run it before patchShebangs would come to the output.
  postPatch = lib.optionalString withUrt ''
    patchShebangs urt/urt-browse
  '';

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

  # Firefox hangs at the start when its home cannot be written (the
  # sandbox's is /homeless-shelter), and WebKit grumbles; give them one.
  preCheck = lib.optionalString withUrt ''
    export HOME="$NIX_BUILD_TOP/home"
    mkdir -p "$HOME"
  '';

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
    install -m 755 urt/urt urt/urt-format urt/urt-browse "$out/bin"
    wrapProgram "$out/bin/urt" --set-default URWEB "$out/bin/urweb" ${browserFlags}
    wrapProgram "$out/bin/urt-browse" ${browserFlags}
  ''
  # The engines' clock, which urt's Makefile builds on Linux and macOS:
  # urt-browse looks for it beside itself, as in the tree, and in
  # ../lib/urt
  + lib.optionalString (withUrt && (stdenv.hostPlatform.isLinux || stdenv.hostPlatform.isDarwin)) (
    let
      clock = if stdenv.hostPlatform.isDarwin then "liburtclock.dylib" else "liburtclock.so";
    in
    ''
      install -D -m 644 urt/${clock} "$out/lib/urt/${clock}"
    ''
  )
  + ''
    runHook postInstall
  '';

  passthru.configureEnv = configureEnv;

  # What urt's browser runs need, for shell.nix
  passthru.urt = {
    python = urtPython;
    browsers = urtBrowsers;
    env = browserEnv;
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
