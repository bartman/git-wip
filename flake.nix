{
  description = "git-wip — Work In Progress branch manager";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          name = "git-wip-dev";

          # Build tools and dependencies
          # Nix name          ↔  apt name
          # cmake             ↔  cmake
          # ninja             ↔  ninja-build
          # pkg-config        ↔  pkg-config
          # gnumake           ↔  make
          # gcc / stdenv      ↔  gcc / g++
          # clang-tools       ↔  clangd
          # clang             ↔  clang
          # libgit2           ↔  libgit2-dev
          # gtest             ↔  googletest / libgmock-dev / libgtest-dev
          # git               ↔  git
          packages = with pkgs; [
            # build system
            cmake
            ninja
            pkg-config
            gnumake

            # compilers
            gcc
            clang
            clang-tools   # provides clangd

            # runtime library (required at link time)
            libgit2

            # test framework
            gtest

            # version control (needed by cmake FetchContent and tests)
            git

            # python is used by test/runner.py
            python3
          ];

          # Ensure pkg-config can find libgit2
          PKG_CONFIG_PATH = "${pkgs.libgit2}/lib/pkgconfig";

          shellHook = ''
            echo "git-wip dev shell"
            echo "  compiler:  $(c++ --version | head -1)"
            echo "  cmake:     $(cmake --version | head -1)"
            echo "  libgit2:   $(pkg-config --modversion libgit2)"
            echo ""
            echo "  build:     make"
            echo "  test:      make test"
          '';
        };
      }
    );
}
