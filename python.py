import os

# Tamaño objetivo en bytes (5 GB)
target_size = 5 * 1024 * 1024 * 1024  # 5 GB

# Bloque de texto base
lorem_block = (
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. "
    "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\n"
)

block_size = len(lorem_block.encode("utf-8"))  # Tamaño real del bloque en bytes

# Ruta del archivo a generar
output_path = "file1.txt"

with open(output_path, "w", encoding="utf-8") as f:
    written = 0
    while written < target_size:
        f.write(lorem_block)
        written += block_size

print(f"Archivo generado: {output_path} ({written / (1024 ** 3):.2f} GB)")
