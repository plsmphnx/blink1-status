{
  inputs = {
    astal = {
      url = "github:aylur/astal";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs = { self, nixpkgs, astal }: let
    systems = fn: nixpkgs.lib.mapAttrs fn nixpkgs.legacyPackages;
    astal-wp = system: astal.packages.${system}.wireplumber;
  in {
    packages = systems (system: pkgs: rec {
      blink1-tool = pkgs.blink1-tool.overrideAttrs (_: old: {
        makeFlags = old.makeFlags ++ [ "GIT_TAG_RAW=v${old.version}" ];
      });

      libblink1 = blink1-tool.overrideAttrs (_: old: {
        pname = "libblink1";
        buildFlags = "lib";
        installTargets = "install-dev";
      });

      default = pkgs.stdenv.mkDerivation {
        name = "blink1-status";
        src = ./.;
        nativeBuildInputs = with pkgs; [ pkg-config ];
        buildInputs = with pkgs; [ (astal-wp system) glib libblink1 ];
        buildPhase = ''
          gcc main.c -o blink1-status \
            $(pkg-config --cflags --libs glib-2.0 gobject-2.0) \
            -l astal-wireplumber -l blink1
        '';
        installPhase = ''
          mkdir -p "$out/bin"
          mv blink1-status "$out/bin"
        '';
      };
    });
  };
}
