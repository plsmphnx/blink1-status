{
  outputs = { nixpkgs, ... }: let
    systems = fn: nixpkgs.lib.mapAttrs (_: fn) nixpkgs.legacyPackages;
  in {
    packages = systems (pkgs: rec {
      blink1-tool = pkgs.blink1-tool.overrideAttrs (_: old: {
        makeFlags = old.makeFlags ++ [ "GIT_TAG_RAW=v${old.version}" ];
      });

      libblink1 = blink1-tool.overrideAttrs (_: old: {
        pname = "libblink1";
        buildFlags = "lib";
        installTargets = "install-dev";
      });

      default = pkgs.stdenv.mkDerivation {
        name = "micblink";
        src = ./.;
        nativeBuildInputs = with pkgs; [ pkg-config ];
        buildInputs = with pkgs; [ astal.wireplumber glib libblink1 ];
        buildPhase = ''
          gcc main.c -o micblink \
            $(pkg-config --cflags --libs glib-2.0 gobject-2.0) \
            -l astal-wireplumber -l blink1
        '';
        installPhase = ''
          mkdir -p "$out/bin"
          mv micblink "$out/bin"
        '';
      };
    });
  };
}
