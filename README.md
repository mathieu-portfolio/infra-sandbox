# infra-sandbox

A C++20/raylib prototype for an educational systems architecture strategy game.

This first version focuses on the simulation and rendering foundations:
clients generate traffic, requests move through links, backend queues build up,
processors complete or time out work, and a debug overlay exposes core metrics.

## Build

```bash
./build.sh
```

Or directly with CMake presets:

```bash
cmake --preset default
cmake --build --preset debug
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

## Controls

- `Space`: pause/resume simulation
- `R`: reset scenario
- `Up` / `Down` or `+` / `-`: increase/decrease client request rate
- `1`: scale backend/database processing capacity up
- `2`: apply a simple cache placeholder that reduces effective client demand
- `3`: reset capacity and cache placeholder
