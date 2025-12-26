#### Install with nix
You can install tail-tray with the Nix-Package Manager or on NixOS via the configuration.nix.

##### Nix-Package Manager
```bash
# persistent on NixOS
nix-env -iA nixos.tail-tray nixos.tailscale

# persistent on non NixOS
nix-env -iA nixpkgs.tail-tray nixpkgs.tailscale

# non persistent
nix-shell -p tail-tray tailscale
```

##### NixOS
configuration.nix:
```nix
{pkgs, ...}:

{
...
    environment.systemPackages = with pkgs; [
        tailscale
        tail-tray
    ];
...
}
```
