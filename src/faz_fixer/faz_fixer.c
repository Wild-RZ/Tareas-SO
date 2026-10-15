#include <stdlib.h>
#include "faz_fixer.h"

// Función principal de reparación (A IMPLEMENTAR POR EL ESTUDIANTE)
void faz_scan_and_fix(FILE *memory_file) {
    // TIPS DE FAZBEAR ENTERTAINMENT:
    // 1. Usa fseek(memory_file, OFFSET_PCBS, SEEK_SET) para mover el cursor.
    // 2. Usa fread() para leer bloques enteros (ej. leer un osrmsPCB completo).
    // 3. Usa las macros GET_BIT, SET_BIT y CLEAR_BIT para auditar/reparar los bitmaps.
    // 4. No olvides usar fwrite() para guardar los cambios en la memoria.
    
    printf("[FAZ_FIXER] Escaneo de invariantes iniciado...\n");
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