# Distribution folder

This folder contains helper scripts to prepare a distributable package of the `ClanManager` app.

Files
- `install.sh` — copies the built `ClanManager.app` from `build/` into `distribution/ClanManager/`.

How to use
1. Build the app (see top-level `README.md`).
2. From the project root run:

```bash
./distribution/install.sh
```

3. Create a tarball for distribution (optional):

```bash
cd distribution
tar -czf ClanManager.tar.gz ClanManager
```

Notes
- This is a simple packaging helper. For a proper macOS `.app` bundle signing and notarization step, follow Apple/Xcode guidelines.
