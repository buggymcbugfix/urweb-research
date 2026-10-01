# nix-build: checks the proofs; the .vo files are the output.
{
  pkgs ? import ./nixpkgs.nix,
}:
pkgs.stdenv.mkDerivation {
  pname = "featherweight-ur";
  version = "0";

  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [
      ./Makefile
      ./_CoqProject
      (pkgs.lib.fileset.fileFilter (f: f.hasExt "v") ./.)
    ];
  };

  nativeBuildInputs = with pkgs.rocqPackages; [
    rocq-core
    stdlib
  ];

  installPhase = ''
    runHook preInstall
    mkdir -p "$out"
    cp *.vo "$out"
    runHook postInstall
  '';
}
