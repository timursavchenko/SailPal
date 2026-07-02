# SailPal

Console-only SailPal server built on NexusCore.

## Build

```sh
./build.sh --config Debug --platform macOS --arch x86_64
```

The build output is copied to `_out/artifacts/application/<config-platform-arch>/SailPalServer`.

## Build And Run (Nexus-style)

```sh
./server.sh --config Debug --platform macOS --arch x86_64 --storage /absolute/path/to/storage --host 127.0.0.1 --port 8080
```

Run modes:

- `./server.sh --build-only --config Debug --platform macOS --arch x86_64`
- `./server.sh --run-only --config Debug --platform macOS --arch x86_64 --storage /absolute/path/to/storage`

## Run

```sh
_out/artifacts/application/debug-macOS-x86_64/SailPalServer --storage /absolute/path/to/storage --host 127.0.0.1 --port 8080
```

Use `--help` to inspect all supported options.

You can also use `./server.sh --help` for combined build+run options.

## VS Code CMake Tools

Project is configured to work via CMake Tools in the same style as Nexus.

1. Open command palette and run `CMake: Select Configure Preset`.
2. Pick preset, for example `macos-x86_64-debug`.
3. Run `CMake: Configure` then `CMake: Build`.
4. Set launch config `Run SailPalServer` and start debugging.

VS Code config files are in `.vscode/`:

- `settings.json`
- `cmake-kits.json`
- `launch.json`
- `tasks.json`
