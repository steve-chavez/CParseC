let
  lock = builtins.fromJSON (builtins.readFile ./flake.lock);
  nixpkgsLock = lock.nodes.${lock.nodes.root.inputs.nixpkgs}.locked;
in
{ pkgs ? import (builtins.fetchTarball {
    name = nixpkgsLock.rev;
    url = "https://github.com/${nixpkgsLock.owner}/${nixpkgsLock.repo}/archive/${nixpkgsLock.rev}.tar.gz";
    sha256 = nixpkgsLock.narHash;
  }) {}
}:
with pkgs;
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
    ];
  shellHook = ''
    export HISTFILE=.history
  '';
}
