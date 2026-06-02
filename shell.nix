with import (builtins.fetchTarball {
  name = "2025-06-16";
  url = "https://github.com/NixOS/nixpkgs/archive/e6f23dc08d3624daab7094b701aa3954923c6bbb.tar.gz";
  sha256 = "sha256:0m0xmk8sjb5gv2pq7s8w7qxf7qggqsd3rxzv3xrqkhfimy2x7bnx";
}) {};
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
    ];
  shellHook = ''
    export HISTFILE=.history
  '';
}
