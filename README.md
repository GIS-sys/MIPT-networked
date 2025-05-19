# How to install

1) clone the repository, cd into it

2)
```bash
git submodule update --init --recursive
git submodule sync
git submodule update --recursive --remote
```

# How to run

## w2

1)
```bash
./compile.sh
```

2) Launch servers and client

```bash
./build/w2/w2_lobby
```

```bash
./build/w2/w2_server
```

```bash
./build/w2/w2_client
```
