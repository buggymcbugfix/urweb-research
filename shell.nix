{ pkgs ? import ./nixpkgs.nix }:
let
  # The shell takes the compiler's inputs from its package and builds it
  # in the tree, where make reads the commit itself; the package's is not
  # read, so that toolchainId does not change with every commit.
  urweb = pkgs.callPackage ./derivation.nix { rev = "?"; };
  # Same derivation minus the sources: its outPath changes iff a reconfigure is needed.
  toolchainId = builtins.unsafeDiscardStringContext (urweb.overrideAttrs (_: { src = null; })).outPath;
in
pkgs.mkShell {
  inputsFrom = [ urweb ];

  packages = with pkgs; [
    git # read by make for `urweb -version`
    python3 # urt's pty cases and stand-in server
    rlwrap
    smlnj
    sqlite
  ];

  shellHook = ''
    export DEVPREFIX="$PWD/out"
    ${urweb.configureEnv "$DEVPREFIX"}
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