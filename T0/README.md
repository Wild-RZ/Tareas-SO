# FAZWATCH - código base

Condigo inicial para iniciar la tarea
## Contenido

- `fazwatch.c`: codigo base inicial
- `protocol.h`: constantes compartidas por FAZWATCH y las cámaras.
- `cameras/camera_normal.c`: envía heartbeats indefinidamente.
- `cameras/camera_short.c`: envía una cantidad finita de heartbeats y termina.
- `cameras/camera_silent.c`: envía algunos heartbeats y luego permanece viva sin enviar más.
- `cameras/camera_stubborn.c`: deja de enviar heartbeats e ignora `SIGTERM`.
- `cameras/camera_crash.c`: envía algunos heartbeats y luego termina por `SIGSEGV`.
- `Makefile`: compila FAZWATCH y todas las cámaras públicas. 

## Compilación

```bash
make
```

