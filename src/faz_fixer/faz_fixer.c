#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "faz_fixer.h"

// Estructura auxiliar para ordenar los procesos activos por process_id (de menor a mayor)
typedef struct {
    int pcb_idx;
    uint8_t pid;
} ActiveProcess;

int compare_procs(const void *a, const void *b) {
    ActiveProcess *pa = (ActiveProcess *)a;
    ActiveProcess *pb = (ActiveProcess *)b;
    return (int)pa->pid - (int)pb->pid;
}

void faz_scan_and_fix(FILE *memory_file) {
    printf("[FAZ_FIXER] Escaneo de invariantes iniciado...\n");

    osrmsPCB pcbs[MAX_PROCESSES];
    uint8_t bitmap_l2[MAX_L2_TABLES / 8];
    osrmsL2Table tables_l2[MAX_L2_TABLES];
    uint8_t frame_bitmap[TOTAL_FRAMES / 8];

    // 1. Cargar todas las estructuras de metadatos en memoria local
    fseek(memory_file, OFFSET_PCBS, SEEK_SET);
    if (fread(pcbs, sizeof(osrmsPCB), MAX_PROCESSES, memory_file) != MAX_PROCESSES) return;

    fseek(memory_file, OFFSET_BITMAP_L2, SEEK_SET);
    if (fread(bitmap_l2, 1, sizeof(bitmap_l2), memory_file) != sizeof(bitmap_l2)) return;

    fseek(memory_file, OFFSET_TABLES_L2, SEEK_SET);
    if (fread(tables_l2, sizeof(osrmsL2Table), MAX_L2_TABLES, memory_file) != MAX_L2_TABLES) return;

    fseek(memory_file, OFFSET_FRAME_BITMAP, SEEK_SET);
    if (fread(frame_bitmap, 1, sizeof(frame_bitmap), memory_file) != sizeof(frame_bitmap)) return;

    // 2. Identificar procesos activos y ordenarlos por process_id ascendente
    // Esto garantiza que el proceso con menor ID reclame primero los PFNs en caso de colision
    ActiveProcess active_procs[MAX_PROCESSES];
    int num_active = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (pcbs[i].state == 0x01) {
            active_procs[num_active].pcb_idx = i;
            active_procs[num_active].pid = pcbs[i].process_id;
            num_active++;
        }
    }

    qsort(active_procs, num_active, sizeof(ActiveProcess), compare_procs);

    // Arreglos para rastrear el estado real (esperado) de las tablas L2 y frames
    bool pfn_claimed[TOTAL_FRAMES] = {false};
    bool l2_used[MAX_L2_TABLES] = {false};

    // 3. Auditar rutas de archivos y resolver colisiones
    for (int a = 0; a < num_active; a++) {
        int i = active_procs[a].pcb_idx;

        for (int f = 0; f < 5; f++) {
            if (pcbs[i].files[f].validity != 0x01) {
                continue; // Ignorar entradas de archivo inactivas
            }

            uint32_t fsize = pcbs[i].files[f].file_size;
            uint32_t vaddr = pcbs[i].files[f].virtual_address;

            // N = ceil(file_size / 4096)
            uint32_t num_pages = (fsize + FRAME_SIZE - 1) / FRAME_SIZE;
            uint32_t start_vpn = (vaddr >> 12) & 0xFFF;

            bool file_valid = true;

            // Fase de verificacion: comprobar que toda la ruta del archivo sea valida y sin colisiones
            for (uint32_t p = 0; p < num_pages; p++) {
                uint32_t vpn = start_vpn + p;
                if (vpn >= 4096) { // Fuera del espacio virtual de 16 MB (4096 paginas)
                    file_valid = false;
                    break;
                }

                uint32_t l1_idx = (vpn >> 6) & 0x3F;
                uint32_t l2_idx = vpn & 0x3F;

                uint16_t sptn = pcbs[i].page_table_l1[l1_idx];
                if (sptn >= MAX_L2_TABLES) { // Ruta invalida en L1
                    file_valid = false;
                    break;
                }

                uint16_t pfn = tables_l2[sptn].entries[l2_idx];
                if (pfn >= TOTAL_FRAMES) { // Ruta invalida en L2
                    file_valid = false;
                    break;
                }

                // Verificar colision con un proceso de menor ID (o consigo mismo)
                if (pfn_claimed[pfn]) {
                    file_valid = false;
                    break;
                }
            }

            // Si hay colision o ruta corrupta, invalidamos la entrada del archivo perdedor
            if (!file_valid) {
                pcbs[i].files[f].validity = 0x00;
                printf("[FAZ_FIXER] Colision o ruta invalida detectada en PCB %d (ID %u). Archivo '%.*s' invalidado.\n",
                       i, pcbs[i].process_id, 14, pcbs[i].files[f].file_name);
            } else {
                // El archivo es 100% valido: marcamos sus tablas L2 y PFNs como legitimamente ocupados
                for (uint32_t p = 0; p < num_pages; p++) {
                    uint32_t vpn = start_vpn + p;
                    uint32_t l1_idx = (vpn >> 6) & 0x3F;
                    uint32_t l2_idx = vpn & 0x3F;

                    uint16_t sptn = pcbs[i].page_table_l1[l1_idx];
                    uint16_t pfn = tables_l2[sptn].entries[l2_idx];

                    l2_used[sptn] = true;
                    pfn_claimed[pfn] = true;
                }
            }
        }
    }

    // 4. Sincronizar Bitmap de Tablas de 2do Orden (Invariantes I1 e I3)
    uint8_t new_bitmap_l2[MAX_L2_TABLES / 8];
    memset(new_bitmap_l2, 0, sizeof(new_bitmap_l2));

    for (int t = 0; t < MAX_L2_TABLES; t++) {
        int old_bit = GET_BIT(bitmap_l2, t);
        int new_bit = l2_used[t] ? 1 : 0;

        if (new_bit) {
            SET_BIT(new_bitmap_l2, t);
        }

        if (old_bit == 0 && new_bit == 1) {
            printf("[FAZ_FIXER] Bitmap L2 desincronizado. Tabla L2 %d marcada como ocupada.\n", t);
        } else if (old_bit == 1 && new_bit == 0) {
            printf("[FAZ_FIXER] Tabla L2 huerfana detectada. Tabla L2 %d liberada.\n", t);
        }
    }

    // 5. Sincronizar Frame Bitmap (Invariantes I2 e I3)
    uint8_t new_frame_bitmap[TOTAL_FRAMES / 8];
    memset(new_frame_bitmap, 0, sizeof(new_frame_bitmap));

    for (int f = 0; f < TOTAL_FRAMES; f++) {
        int old_bit = GET_BIT(frame_bitmap, f);
        int new_bit = pfn_claimed[f] ? 1 : 0;

        if (new_bit) {
            SET_BIT(new_frame_bitmap, f);
        }

        if (old_bit == 0 && new_bit == 1) {
            printf("[FAZ_FIXER] Bitmap desincronizado. PFN %d marcado como ocupado.\n", f);
        } else if (old_bit == 1 && new_bit == 0) {
            printf("[FAZ_FIXER] Fuga de memoria detectada. Frame fisico %d liberado.\n", f);
        }
    }

    // 6. Escribir los cambios de vuelta en el archivo binario reparable
    fseek(memory_file, OFFSET_PCBS, SEEK_SET);
    fwrite(pcbs, sizeof(osrmsPCB), MAX_PROCESSES, memory_file);

    fseek(memory_file, OFFSET_BITMAP_L2, SEEK_SET);
    fwrite(new_bitmap_l2, 1, sizeof(new_bitmap_l2), memory_file);

    fseek(memory_file, OFFSET_FRAME_BITMAP, SEEK_SET);
    fwrite(new_frame_bitmap, 1, sizeof(new_frame_bitmap), memory_file);

    fflush(memory_file);
    printf("[FAZ_FIXER] Memoria reparada guardada exitosamente.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <mem_corrupta.bin> <mem_reparada.bin>\n", argv[0]);
        return 1;
    }

    // Copiamos el archivo corrupto a uno nuevo para no destruir la evidencia original
    FILE *src = fopen(argv[1], "rb");
    FILE *dst = fopen(argv[2], "wb");
    if (!src || !dst) {
        perror("Error al abrir archivos para copia");
        if (src) fclose(src);
        if (dst) fclose(dst);
        return 1;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    fclose(src);
    fclose(dst);

    // Abrimos la copia reparable en modo binario de lectura/escritura
    FILE *mem_file = fopen(argv[2], "r+b");
    if (!mem_file) {
        perror("Error al abrir el archivo de memoria reparable");
        return 1;
    }

    faz_scan_and_fix(mem_file);

    fclose(mem_file);
    return 0;
}