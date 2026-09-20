# New Release Checklist

> Commit relevant release changes after each related phase, keeping each commit limited to that phase and using a clear message. Suggested groups include minifig settings, CI/library links, and version/documentation updates. Keep this checklist uncommitted when it is being used as private working notes. Do not mark steps as completed in this file.

1. Update Minifig Wizard settings:
   - Go to [MLCad INI Download](https://www.ldraw.org/mlcad-ini-download.html) and download the latest ZIP file.
   - Unzip it and update `resources/minifig.ini` with the new `MLCad.ini`.
   - Keep the existing minifig sections; exclude unrelated sections such as `[SCAN_ORDER]` and `[LSYNTH]`.
   - Keep the version, support, and acknowledgements comments at the top; trim other comments and extra spaces.

2. Update color definitions:
   - Download [`LDConfig.ldr`](https://library.ldraw.org/library/official/LDConfig.ldr).
   - Copy it to `resources/ldconfig.ldr`.

3. Update `resources/ldraw.xml`

4. Update `resources/traintrack.json`

5. Update Parts Library:
   - Go to [LDraw library updates](https://library.ldraw.org/updates?latest) and download `complete.zip`.
   - Rename it to `library.bin`.
   - Zip `library.bin` as `Library-YY.MM.zip`, using the month and year shown on the updates page.
   - Verify the resulting ZIP contains the complete, valid `library.bin` and is not truncated.
   - Open the latest GitHub release in the default web browser and open the generated ZIP in the default file browser.
   - Manually upload the resulting ZIP to the latest GitHub release.
   - Update the library ZIP links in `appveyor.yml` and `.github/workflows/continuous.yml` to the old release.

6. Update the release version in the current `YY.MM` format:
   - `leocad.pro`
   - `common/lc_global.h`
   - `qt/Info.plist`
   - `snapcraft.yaml`
   - `leocad.spec`
   - `tools/setup/leocad.appdata.xml`
   - In `tools/setup/leocad.appdata.xml`, add the new release entry; do not replace the existing release history.

7. Update `docs/README.md` and `docs/leocad.1`

8. Configure the release build and artifacts:
   - Set skip_tags to false in `appveyor.yml`.
   - Add a GitHub tag for the new release.
   - Rename the packages uploaded by AppVeyor.
   - Download and save Windows symbols.

9. Create release notes:
   - Check for changes with `git log vYY.MM..HEAD`.

10. Update the website:
   - Update the version history on the website.
   - Add the new version and parts to `updates.txt`.
   - Update download links.

11. Complete post-release CI cleanup:
   - Remove the build-on-tags setting from `appveyor.yml`.
   - Update the library ZIP links in `appveyor.yml` and `.github/workflows/continuous.yml` to the new release.

12. Update the [official Flathub LeoCAD repository](https://github.com/flathub/org.leocad.LeoCAD):
   - If you do not have write access, fork this official repository; do not use a fork of the generic `flathub/flathub` repository.
   - Update [`org.leocad.LeoCAD.json`](https://github.com/flathub/org.leocad.LeoCAD/blob/master/org.leocad.LeoCAD.json) to the new LeoCAD tag, commit, and library ZIP URL.
   - Update files under [`patches/`](https://github.com/flathub/org.leocad.LeoCAD/tree/master/patches) if required, then submit the Flathub changes.

13. Update the Homebrew cask:
   - Update [`Casks/l/leocad.rb`](https://github.com/Homebrew/homebrew-cask/blob/HEAD/Casks/l/leocad.rb) in the Homebrew cask repository.
   - Set the new version and SHA-256 for `LeoCAD-macOS-YY.MM.dmg`.
   - Resolve the current Gatekeeper issue if it still disables the cask.

14. Update the Windows package manager:
   - Update package ID `LeonardoZide.LeoCAD` in `microsoft/winget-pkgs`.
   - Use the [existing manifest folder](https://github.com/microsoft/winget-pkgs/tree/master/manifests/l/LeonardoZide/LeoCAD/25.09) as the template and add the new three-file manifest set under `manifests/l/LeonardoZide/LeoCAD/<version>/`.
   - Follow the [WinGet authoring instructions](https://github.com/microsoft/winget-pkgs/blob/master/doc/Authoring.md), validate the manifest, and submit a pull request.

15. Update Snapcraft on Linux:
   - Run `snapcraft --use-lxd`.
   - Upload the resulting snap with `snapcraft upload --release=stable mysnap_latest_amd64.snap`.
