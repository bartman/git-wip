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

                # Read the canonical version from the committed VERSION file
                # and combine it with flake-provided metadata to produce a
                # string of the same shape as cmake/GitVersion.sh:
                #   {VERSION}-{YYYYMMDD}-g{HASH}[-dirty]
                baseVersion = pkgs.lib.fileContents ./VERSION;
                # self.lastModifiedDate is "YYYYMMDDHHMMSS" — take the date.
                buildDate = builtins.substring 0 8 (self.lastModifiedDate or "00000000");
                shortHash = self.shortRev or self.dirtyShortRev or "unknown";
                dirtySuffix = if self ? rev then "" else "-dirty";
                gitWipVersion = "${baseVersion}-${buildDate}-g${shortHash}${dirtySuffix}";
                in
                {
                packages.default = pkgs.callPackage ./nix/package.nix {
                    inherit pkgs;
                    version = gitWipVersion;
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
