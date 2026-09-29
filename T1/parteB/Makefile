###############################################################################
#                             Makefile Parte B                                #
#                                                                             #
# Compilación directa de SRTN y módulos locales                              #
###############################################################################

# Compilador y flags
CC = gcc -std=gnu11
CFLAGS = -Wunused -Wall -g

# Librerías
LIB = -lm

# Nombre del ejecutable final
TARGET = SRTN best_SRTN


# Directorio para archivos objeto
OBJDIR = obj

# Buscar todos los archivos .c y .h dentro de esta carpeta y subcarpetas (excluyendo obj/)
SRCFILES := $(shell find . -maxdepth 2 -name '*.c' ! -path './obj/*')
HDRFILES := $(shell find . -maxdepth 2 -name '*.h' ! -path './obj/*')

# Convertir archivos .c en .o mapeados dentro de la carpeta obj/
OBJFILES := $(patsubst ./%.c, $(OBJDIR)/%.o, $(SRCFILES))

# Generar la lista de subdirectorios dentro de obj/ para mantener la estructura
OBJSUBDIRS := $(sort $(dir $(OBJFILES)))

###############################################################################
# REGLAS                                                                      #
###############################################################################

all: $(TARGET)
	@echo "Compilación exitosa: ./$(TARGET)"

# Crear el ejecutable uniendo todos los objetos


# Compilar cada .c individual a su correspondiente .o
$(OBJDIR)/%.o: %.c $(HDRFILES) | create_dirs
	@$(CC) $(CFLAGS) $< -c -o $@ $(LIB) && echo "Compilado '$<'"

# Crear carpetas necesarias para los objetos
create_dirs:
	@mkdir -p $(OBJSUBDIRS)

clean:
	@rm -rf $(OBJDIR) $(TARGET) /tmp/srtn_test_out.txt && echo "Limpieza completada."

