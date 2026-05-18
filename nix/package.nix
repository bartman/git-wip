{ stdenv
, pkgs
, cmake
, ninja
, pkg-config
, git
, libgit2
, spdlog
, openssl
, pcre2
, libssh2
, zlib
}:

stdenv.mkDerivation {
    pname = "git-wip";
    version = "unstable-";

    src = ./..;

    nativeBuildInputs = [ cmake ninja pkg-config git ];

    buildInputs = [ libgit2 libgit2.dev spdlog openssl openssl.dev pcre2 libssh2 zlib ];

    PKG_CONFIG_PATH = with pkgs; lib.makeSearchPath "lib/pkgconfig" [
        openssl.dev libgit2 pcre2 libssh2 zlib
    ];

    postPatch = ''
        patchShebangs cmake/GitVersion.sh
        '';

    preConfigure = ''
        echo "=== Generating git-wip version header for Nix build ==="
        mkdir -p build
        ./cmake/GitVersion.sh GIT_WIP_ build/git_wip_version.h
        ls -l build/git_wip_version.h
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
}
