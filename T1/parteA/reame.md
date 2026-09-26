# Tarea 1 - Parte A: Diagnóstico y Corrección

## 1. Problema de diseño 1: Reseteo del progreso en ráfagas de CPU
* **Diagnóstico:** En el ciclo principal (`main.c`), cuando un proceso en estado `RUNNING` agotaba su *quantum*, el código original reseteaba su variable `burst_progress` a 0.
* **Inconsistencia:** Esto contradice directamente la Sección 5 del enunciado, que establece que "El progreso dentro de una ráfaga es independiente del quantum" y que debe conservarse para reanudar la ejecución desde ese punto.
* **Corrección aplicada:** Se eliminó la línea `cpu->burst_progress = 0;` del bloque `else if (cpu->quantum_remaining == 0)`.

## 2. Problema de diseño 2: Orden incorrecto del flujo del scheduler
* **Diagnóstico:** La revisión para cambiar los procesos al estado `DEAD` (cuando el `tick` actual alcanzaba el `t_deadline`) se estaba ejecutando al final del ciclo `while`, después del ordenamiento y del *dispatch*.
* **Inconsistencia:** Esto viola la jerarquía de prioridades por tick estipulada en la Sección 4 del enunciado. La "Verificación de deadlines" debe realizarse estrictamente antes del "Ordenamiento" y la "Ejecución/Dispatch". Si no, un proceso vencido podría ser asignado a la CPU.
* **Corrección aplicada:** Se trasladó todo el bloque que itera sobre la `queue` para verificar los *deadlines* hacia arriba, posicionándolo justo antes de la invocación a `queue_sort(&queue)`.

## 3. Evidencia Experimental
Antes de aplicar las correcciones, los procesos con ráfagas de CPU superiores al tamaño del *quantum* entraban en un ciclo de ejecución infinito, ya que su progreso se borraba constantemente, resultando en un *turnaround time* incorrecto o en procesos que nunca alcanzaban el estado `FINISHED`. Tras aplicar el parche, los procesos conservan su progreso, terminan sus ráfagas correctamente y pasan los tests automáticos (Easy, Medium y Hard) entregando métricas de *turnaround*, *response time* y número de interrupciones coherentes con el algoritmo EDF+RR.