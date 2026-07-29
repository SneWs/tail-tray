# Flatpak via Flathub

This project targets Flathub as the official Flatpak distribution channel.

## Scope

- Do not publish `.flatpak` bundles from this repository's GitHub Releases.
- Publish through Flathub so users can discover and install from standard Flatpak app stores.

## Recommended rollout

1. Ensure Flatpak metadata is complete for Flathub review:
   - Stable app ID (reverse DNS style)
   - AppStream metainfo (`.metainfo.xml`)
   - Desktop file alignment with app ID
   - Icons available in required sizes
2. Keep the Flatpak manifest in `packaging/flatpak/io.github.SneWs.TailTray.yml` as upstream reference.
3. Use `io.github.SneWs.TailTray` consistently as the Flatpak app ID, desktop ID, metainfo ID, and primary icon name.
4. Submit the app to Flathub by opening a PR in the Flathub repository with the manifest and metadata.
5. Iterate on Flathub review comments until accepted.

## Flathub PR payload (exact structure)

When creating the PR in the Flathub repository, include:

1. `io.github.SneWs.TailTray.yml`
2. Any required patches as separate files (only if Flathub asks)
3. Optional `flathub.json` if reviewers request additional policy metadata

The manifest should use the same app ID as this repository and pull from a fixed upstream release tag/commit.

`sources` section example for Flathub submission:

```yaml
sources:
  - type: git
    url: https://github.com/SneWs/tail-tray.git
    tag: v0.2.34
    commit: <release-commit-sha>
```

## Local validation before opening Flathub PR

Run these from the project root:

```bash
cmake -S . -B build-flathub-validate -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DKNOTIFICATIONS_ENABLED=ON
cmake --install build-flathub-validate --prefix /tmp/tail-tray-stage
desktop-file-validate /tmp/tail-tray-stage/share/applications/io.github.SneWs.TailTray.desktop
appstreamcli validate --pedantic io.github.SneWs.TailTray.metainfo.xml
flatpak-builder --show-manifest packaging/flatpak/io.github.SneWs.TailTray.yml > /dev/null
```

## After Flathub acceptance

Update `README.md` to link directly to the official Flathub app page.

