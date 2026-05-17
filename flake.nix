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
                packages.default = pkgs.stdenv.mkDerivation {
                    pname = "git-wip";
                    version = "unstable-${self.shortRev or self.dirtyShortRev or "dirty"}";

                    src = self;

                    nativeBuildInputs = with pkgs; [
                    cmake
                    ninja
                    pkg-config
                    git
                    ];

                    buildInputs = with pkgs; [
                    libgit2
                    libgit2.dev
                    spdlog
                    openssl
                    openssl.dev
                    pcre2
                    libssh2
                    zlib
                    ];

                    PKG_CONFIG_PATH = with pkgs; lib.makeSearchPath "lib/pkgconfig" [
                        openssl.dev libgit2 pcre2 libssh2 zlib
                    ];

# Better phase for patching shebangs
                    postPatch = ''
                        patchShebangs cmake/GitVersion.sh
                        '';

# Pre-generate version header
                    preConfigure = ''
                        echo "=== Generating git-wip version header for Nix build ==="
                        mkdir -p build
                        ./cmake/GitVersion.sh GIT_WIP_ build/git_wip_version.h
                        '';

                    cmakeFlags = [
                        "-DCMAKE_BUILD_TYPE=Release"
                            "-DBUILD_TESTING=OFF"
                            "-DUSE_GIT_WIP_VERSION_H=${placeholder "source"}/build/git_wip_version.h"
                    ];

                    buildPhase = ''
                        make -j$NIX_BUILD_CORES
                        '';

                    installPhase = ''
                        make install PREFIX=$out
                        '';

                    meta = with pkgs.lib; {
                        description = "git-wip — Work In Progress branch manager";
                        homepage = "https://github.com/bartman/git-wip";
                        license = licenses.gpl2Only;
                        platforms = platforms.linux ++ platforms.darwin;
                    };
                };

                devShells.default = pkgs.mkShell {
                    name = "git-wip-dev";
                    packages = with pkgs; [
                        cmake ninja pkg-config gnumake
                            gcc clang clang-tools
                            libgit2 gtest git python3
                    ];
                    PKG_CONFIG_PATH = "${pkgs.libgit2}/lib/pkgconfig";
                    shellHook = ''
                        echo "git-wip dev shell ready"
                        echo "  build:  make"
                        echo "  test:   make test"
                        echo "  install (local): make install"
                        '';
                };
                });
}
