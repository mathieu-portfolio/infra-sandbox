# infra-sandbox

A C++20/raylib prototype for an educational systems architecture strategy game.

This first version focuses on the simulation and rendering foundations:
clients generate traffic, requests move through links, backend queues build up,
processors complete or time out work, and a debug overlay exposes core metrics.
The current simulation distinguishes lightweight requests from database-heavy
requests, models API and database queues separately, supports retries, and has
a small TTL cache for repeat database-heavy work. Scenario definitions now drive
traffic pressure, phases, educational focus, objectives, events, and available mechanics.

## Build

```bash
./build.sh
```

Or directly with CMake presets:

```bash
cmake --preset app-debug
cmake --build --preset app-debug
```

If raylib or GoogleTest are not already installed, CMake will fetch them by default.
Use `-DINFRA_FETCH_RAYLIB=OFF` if you want raylib configuration to fail instead.

## Run

```bash
./run.sh
```

## Tests

```bash
./tests.sh
```

## Architecture

See [docs/architecture.md](docs/architecture.md) for the current split between layers, runtime systems, mechanics, node categories, and overlay modes.

## Controls

- Top phase button: move from observation to planning, validate a plan, then review resolution
- `F1`-`F7`: switch overlay mode
- `F8`: clear overlay mode
- `F11`: bottleneck pressure overlay
- `F12`: retry amplification overlay
- `F9`: toggle debug UI
- `F10`: toggle metrics UI
- Left click a node: select it and show minimal node details
- Right mouse drag: pan camera
- Mouse wheel: zoom camera
