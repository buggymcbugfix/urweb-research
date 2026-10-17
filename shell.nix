{ pkgs ? import ./nixpkgs.nix }:
let
  # The shell takes the compiler's inputs from its package and builds it
  # in the tree, where make reads the commit itself. The package's commit
  # is not read: mkShell compares the package with its inputs by output
  # path, and without a rev that path would read .git at every start.
  urweb = pkgs.callPackage ./derivation.nix { rev = "?"; };
  # The shell apart from its hook
  shellInputs = {
    inputsFrom = [ urweb ];

    packages = with pkgs; [
      git # read by make for `urweb -version`
      urweb.passthru.urt.python # urt's pty cases, stand-in server and browser runs
      rlwrap
      smlnj
      sqlite
    ];
  };
  configureEnv = urweb.configureEnv "$DEVPREFIX";
  # What configure sees: the shell's inputs, which the derivation of the
  # shell without its hook records, and the variables configureEnv exports.
  # The package's own steps (postPatch, checkPhase, ...) are not in it,
  # since nix-shell runs none of them. Nor is the configure command below:
  # after changing it, run make distclean.
  toolchainId = builtins.hashString "sha256" (builtins.unsafeDiscardStringContext
    ((pkgs.mkShell shellInputs).drvPath + configureEnv));
in
pkgs.mkShell {
  inherit (shellInputs) inputsFrom packages;

  # The browsers urt's browser runs drive (derivation.nix)
  env = urweb.passthru.urt.env;

  shellHook = ''
    export DEVPREFIX="$PWD/out"
    ${configureEnv}
    # The toolbox as built in the tree, by `make -C urt`
    export PATH="$PWD/urt:$PATH"

    _urweb_id="${toolchainId} $DEVPREFIX"
    _urweb_stamp="$DEVPREFIX/.configured-with"

    urweb_configure() {
      [ -f config.status ] && [ -f src/c/Makefile ] &&
        [ "$(cat "$_urweb_stamp" 2>/dev/null)" = "$_urweb_id" ] && return 0
      echo "urweb: toolchain or prefix changed, reconfiguring" >&2
      make -k distclean >/dev/null 2>&1
      rm -rf "''${DEVPREFIX:?}" config.status
      ./autogen.sh && ./configure --prefix="$DEVPREFIX" || return
      mkdir -p "$DEVPREFIX" && printf '%s\n' "$_urweb_id" > "$_urweb_stamp"
    }

    repl() {
      urweb_configure && make -C src/c && make -C src/c install && make smlnj || return
      rlwrap -pgreen -- sml .init.sml "$@"
    }

    echo 'Run `repl` to rebuild Ur/Web using smlnj.'
  '';
}
