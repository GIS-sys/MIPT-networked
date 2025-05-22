# What was done

Added time to snapshots

Store snapshots history on client

Add "frame" ids, initialize start from 0 on server, fix dt

Add local simulation and interpolation for reducing latency and lags

## How to test

If you want to see what will happend when both interpolation AND simulation are disabled, set:
utils.h :: static bool DISABLE_SIMULATION_AND_INTERPOLATION = true;

If you want to play with client interpolation delay (how much client lags behind for smooth image), change:
utils.h :: static const uint32_t CLIENT_INTERPOLATION_DELAY_MS

If you want to play with server's "ping" (how much it waits between sending snapshots to clients), change:
utils.h :: static const int SERVER_DELAY_PING_SIMULATION_MS

# How to install

1) clone the repository, cd into it

2)
```bash
git submodule update --init --recursive
git submodule sync
git submodule update --recursive --remote
```

# How to run

## w5

1)
```bash
./compile.sh
```

2) Launch servers and client

```bash
./build/w5/w5_server
```

```bash
./build/w5/w5
```

## Based on:

https://habr.com/ru/articles/328702/
