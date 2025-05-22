# What was done

TODO

static bool DISABLE_SIMULATION_AND_INTERPOLATION = false;
static const uint32_t CLIENT_INTERPOLATION_DELAY_MS = 200;
static const int SERVER_DELAY_PING_SIMULATION_MS = 100;

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
