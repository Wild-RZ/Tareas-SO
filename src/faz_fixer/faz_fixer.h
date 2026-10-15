#pragma once
#include <stdint.h>
#include <stdio.h>

// --- CONSTANTES ARQUITECTONICAS ---
#define OFFSET_PCBS 0
#define OFFSET_BITMAP_L2 8192
#define OFFSET_TABLES_L2 8320
#define OFFSET_FRAME_BITMAP 139392
#define OFFSET_FRAMES 143360

#define FRAME_SIZE 4096
#define MAX_PROCESSES 32
#define MAX_L2_TABLES 1024
#define TOTAL_FRAMES 32768

// --- MACROS PARA MANEJO DE BITMAPS ---
// array: puntero al arreglo de bytes (ej. uint8_t*)
// bit: indice del bit que se desea consultar/modificar (0 a N-1)
#define GET_BIT(array, bit)   (((array)[(bit) / 8] >> (7 - ((bit) % 8))) & 1)
#define SET_BIT(array, bit)   ((array)[(bit) / 8] |= (1 << (7 - ((bit) % 8))))
#define CLEAR_BIT(array, bit) ((array)[(bit) / 8] &= ~(1 << (7 - ((bit) % 8))))

// --- ESTRUCTURAS DE DATOS (Empaquetadas) ---
// Entrada de la Tabla de Archivos (23 Bytes)
typedef struct __attribute__((packed)) {
    uint8_t validity;
    char file_name[14];
    uint32_t file_size;
    uint32_t virtual_address;
} osrmsFileEntry;

// Bloque de Control de Proceso - PCB (256 Bytes)
typedef struct __attribute__((packed)) {
    uint8_t state;
    uint8_t process_id;
    char process_name[11];
    osrmsFileEntry files[5];
    uint16_t page_table_l1[64];
} osrmsPCB;

// Tabla de Paginas de Segundo Orden (128 Bytes)
typedef struct __attribute__((packed)) {
    uint16_t entries[64];
} osrmsL2Table;

// --- FIRMAS DE FUNCIONES A IMPLEMENTAR ---
void faz_scan_and_fix(FILE *memory_file);