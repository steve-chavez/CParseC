let
  lock = builtins.fromJSON (builtins.readFile ./flake.lock);
  nixpkgsLock = lock.nodes.${lock.nodes.root.inputs.nixpkgs}.locked;
  filnixLock = lock.nodes.${lock.nodes.root.inputs.filnix}.locked;
  filnix = builtins.getFlake
    "github:${filnixLock.owner}/${filnixLock.repo}/${filnixLock.rev}?narHash=${filnixLock.narHash}";
in
{ pkgs ? import (builtins.fetchTarball {
    name = nixpkgsLock.rev;
    url = "https://github.com/${nixpkgsLock.owner}/${nixpkgsLock.repo}/archive/${nixpkgsLock.rev}.tar.gz";
    sha256 = nixpkgsLock.narHash;
  }) {},
  filcc ? if builtins.currentSystem == "x86_64-linux" then
    filnix.packages.${builtins.currentSystem}.filcc
  else
    null
}:
with pkgs;
let
  filc =
    if filcc != null then
      [
        (writeShellScriptBin "filc" ''
          exec ${filcc}/bin/clang "$@"
        '')
      ]
    else [];
in
mkShellNoCC {
  buildInputs =
    [
      gcc15
      clang-tools
      (haskellPackages.ghcWithPackages (p: [ p.attoparsec ]))
      p7zip
      hyperfine
      rustc
      cargo
      clang
    ] ++ filc;
  shellHook = ''
    export HISTFILE=.history
  '';
}
