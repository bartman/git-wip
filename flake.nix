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
                packages.default = pkgs.callPackage ./nix/package.nix {
                    #version = "unstable-${self.shortRev or self.dirtyShortRev or "dirty"}";
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
