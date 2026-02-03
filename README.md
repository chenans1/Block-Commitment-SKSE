Block commit

## Quick start

```bash
git clone https://github.com/<you>/SKSE-Plugin-Template
cd SKSE-Plugin-Template
git submodule update --init --recursive
cmake -B build -S .
cmake --build build --config Release
