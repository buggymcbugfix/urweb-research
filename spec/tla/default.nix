# nix-build: checks the model (make check, make bugs); the TLC logs are the
# output.
{
  pkgs ? import ./nixpkgs.nix,
}:
pkgs.stdenv.mkDerivation {
  pname = "urweb-request-model";
  version = "0";

  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [
      ./Makefile
      ./check-bugs.sh
      (pkgs.lib.fileset.fileFilter (f: f.hasExt "tla" || f.hasExt "cfg") ./.)
    ];
  };

  nativeBuildInputs = [ pkgs.tlaplus ];

  buildPhase = ''
    runHook preBuild
    make check bugs
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p "$out"
    cp out/*.log "$out"
    runHook postInstall
  '';
}
