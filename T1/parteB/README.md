# Parte B - Mejorar un scheduler: (Shortest Remaining Time Next)

## 1. Conjunto de procesos de prueba

Se probo un conjunto de 6 procesos representativo de un **perfil intensivo en CPU**:
cada proceso tiene una única ráfaga de CPU (sin I/O intermedio), y todas las ráfagas
son de duración similar entre sí. Además, los procesos llegan muy cerca uno del otro
(una llegada por tick).

| PID | Arrival | Burst |
|-----|---------|-------|
| 1   | 0       | 67    |
| 2   | 1       | 65    |
| 3   | 2       | 63    |
| 4   | 3       | 61    |
| 5   | 4       | 59    |
| 6   | 5       | 57    |

Las ráfagas decrecen levemente (2 ticks) de un proceso al siguiente, y todas están en
un rango acotado (57–67 ticks). Esta leve variación decreciente es intencional: permite
que SRTN sí tenga motivos formales para la interrupcion (el proceso que llega siempre tiene
un remaining_time algo menor que el que está corriendo), pero la diferencia entre
ambos es pequeña en comparación al largo total de la ráfaga.

## 2. Modelo de overhead de cambio de contexto

Si bien en un contexto real toma entre 1 y 10 microsegundos dependiendo del hardware
para poder medir el impacto real del "número de interrupciones", se agregó a ambas
implementaciones un costo fijo `CONTEXT_SWITCH_OVERHEAD = 2` ticks cada vez que la CPU
pasa de ejecutar un proceso a ejecutar **otro proceso distinto** (una interrupcion real).
Durante esos ticks ningún proceso avanza su ejecución. 

## 3. Resultados con SRTN original

Ejecución: `./srtn`

| PID | Arrival | Burst | Completion | Turnaround | Waiting |
|     |  Time   | Time  |   Time     |    Time    |  Time   |
|-----|---------|-------|------------|------------|---------|
| 1   | 0       | 67    | 378        | 378        | 311     |
| 2   | 1       | 65    | 312        | 311        | 246     |
| 3   | 2       | 63    | 248        | 246        | 183     |
| 4   | 3       | 61    | 185        | 182        | 121     |
| 5   | 4       | 59    | 124        | 120        | 61      |
| 6   | 5       | 57    | 66         | 61         | 4       |

**Métricas agregadas:**
- Número de interrupciones: **3**
- Turnaround promedio: **216.33**
- Waiting time promedio: **154.33**

Mirando la traza de ejecución (ver `trace_original.txt`), se observa que en los
primeros ticks la CPU alterna varias veces entre P1, P4 y P7 antes de asentarse en
P8: como cada nuevo proceso que llega tiene un remaining_time apenas un par de ticks
menor que el proceso corriendo, SRTN decide preemptar de todas formas, pagando el
costo de overhead cada vez, para una ganancia muy marginal en el orden de ejecución.

## 4. Debilidad identificada

Cuando las ráfagas de los procesos son de **duración similar** (perfil CPU-intensivo),
las diferencias de remaining_time entre el proceso que está corriendo y los que van
llegando suelen ser pequeñas (en lo que probe y estoy documento, 2 ticks sobre ráfagas de 57 a
67 ticks). SRTN, al interrumpir ante *cualquier* proceso con menor remaining_time,
sin importar que tan pequeña sea la diferencia, termina generando cambios de contexto que:

1. Tienen un costo real (overhead) que no se compensa con una mejora proporcional en
   el turnaround o el waiting time, porque la diferencia de remaining_time que motivó
   la interrupcion era mínima.
2. No aportan al objetivo original de SRTN (minimizar el tiempo de espera promedio),
   ya que ese objetivo se logra igual de bien completando el proceso casi-empatado que
   ya está corriendo.

## 5. Mejora propuesta: SRTN con umbral de interrupcion

Se implementó `best_SRTN.c`, una variante de SRTN que solo preempta al proceso en
ejecución si el candidato disponible tiene un remaining_time **estrictamente menor en
más de `THRESHOLD` ticks** que el proceso corriendo. Si la diferencia es igual o menor
al umbral, se deja continuar al proceso que ya está en CPU, evitando un cambio de
contexto de bajo beneficio.

El umbral usado en los experimentos es `THRESHOLD = 1`.

Ejecución: `./best_SRTN`.

## 6. Resultados con SRTN mejorado (threshold = 1)

| PID | Arrival | Burst | Completion | Turnaround | Waiting |
|     |  Time   | Time  |   Time     |    Time    |  Time   |
|-----|---------|-------|------------|------------|---------|
| 1   | 0       | 67    | 311        | 311        | 244     |
| 2   | 1       | 65    | 376        | 375        | 310     |
| 3   | 2       | 63    | 185        | 183        | 120     |
| 4   | 3       | 61    | 246        | 243        | 182     |
| 5   | 4       | 59    | 123        | 119        | 60      |
| 6   | 5       | 57    | 64         | 59         | 2       |

**Métricas agregadas:**
- Número de interrupciones: **2**
- Turnaround promedio: **215.00**
- Waiting time promedio: **153.00**

## 7. Comparación y justificación

| Métrica                          | SRTN original | SRTN mejorado (threshold=1) | Cambio |
|----------------------------------|:-------------:|:---------------------------:|:------:|
| Número de interrupciones         | 3             | 2                           | -1     |
| Turnaround promedio              | 216.33        | 215.00                      | -1.33  |
| Waiting time promedio            | 154.33        | 153.00                      | -1.33  |

Con un umbral de 1 tick, SRTN mejorado reduce las interrupciones de 3 a 2, y tanto el
turnaround como el waiting time promedio **no empeoran, sino que mejoran levemente**
(bajan 1.33 ticks cada uno).
Se probaron además otros valores de threshold sobre el mismo conjunto de procesos
para entender mejor el comportamiento:

| Threshold | Interrupciones | Turnaround promedio | Waiting time promedio |
|:---------:|:---------------:|:---------------------:|:------------------------:|
| 0 (= original) | 3          | 216.33                | 154.33                   |
| 1         | 2               | 215.00                | 153.00                   |
| 2         | 2               | 216.00                | 154.00                   |
| 3         | 1               | 214.67                | 152.67                   |
| 4         | 1               | 216.17                | 154.17                   |
| 5         | 0               | 215.33                | 153.33                   |
| 6         | 0               | 217.00                | 155.00                   |
| 8         | 0               | 220.33                | 158.33                   |

A diferencia de lo que se podría esperar, subir el threshold no mejora el turnaround
de forma monótona: el mejor resultado del barrido se da en threshold=1–3, con muy
pocas interrupciones (1-2). A partir de threshold=5, donde ya no ocurre ninguna
interrupción, el turnaround empeora progresivamente (215.33 → 220.33): en ese punto
SRTN mejorado se vuelve prácticamente no-preemptivo (equivalente a FCFS)
y deja de aprovechar las pocas interrupciones que sí valían la pena.

## 8. Compilación y ejecución

\`\`\`bash
make              #compila ambos binarios: SRTN y best_SRTN
./SRTN             # corre SRTN original (pide datos por teclado)
./best_SRTN        # corre SRTN mejorado (pide datos + threshold por teclado)
make clean         # elimina los objetos y binarios generados
\`\`\`

Ambos programas piden por teclado el número de procesos, luego arrival time y burst
time de cada uno; `best_SRTN` además pide el valor de threshold a usar. El conjunto de
prueba usado en este informe (Sección 1) se ingresó de la siguiente forma:

\`\`\`
Enter number of processes: 6
Enter arrival and burst time for P1: 0 67
Enter arrival and burst time for P2: 1 65
Enter arrival and burst time for P3: 2 63
Enter arrival and burst time for P4: 3 61
Enter arrival and burst time for P5: 4 59
Enter arrival and burst time for P6: 5 57
Enter number of threshold: 1
\`\`\`

## 9. Referencias

1. GeeksforGeeks. *Shortest Remaining Time First (Preemptive SJF) Scheduling
   Algorithm*.
   https://www.geeksforgeeks.org/operating-systems/shortest-remaining-time-first-preemptive-sjf-scheduling-algorithm/

2. Simple Snippets. *Algoritmo de programación de CPU de tiempo restante más corto
   (SRT) - Sistemas operativos* [Video, doblaje automático de YouTube]. YouTube.
   https://www.youtube.com/watch?v=Nr4nANJjddg