# CG PROJECT

## Manual
`main.exe image_path` path to image
`-c r/g/b/a/a` color of rain, rgba in range 0.0-1.0
`-n rain_count` number of rain drops, in float
`-s speed` falling speed, in float

## Build
```
cmake . -Bbuild
cmake --build build
```

## Run

UNIX?
```
./build/main
```

Windows (MSVC)
```
./build/Debug/main.exe
```
