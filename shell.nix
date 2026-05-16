# Book restricts to a prior version of dart
# how I found previous version of dard https://www.nixhub.io/packages/dart
{ pkgs ? import (fetchTarball {
  url = "https://github.com/NixOS/nixpkgs/archive/e040aab15638aaf8d0786894851a2b1ca09a7baf.tar.gz";
  sha256 = "01zbkazzfnp1lq7g5zk9s1xh53jwsny9aabng879n8csh1i8qbmk";
}) {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.dart
    pkgs.zulu
    pkgs.gcc
    pkgs.gnumake
    pkgs.git
    pkgs.glibcLocales
  ];

  shellHook = ''
    export LANG=en_US.UTF-8
  '';
}
