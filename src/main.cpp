#include "include/compressor.h"
#include "include/decompressor.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

// ── Helpers ───────────────────────────────────────────────────────────────────

static int prompt_int(const std::string& label, int min_val, int max_val)
{
    int value = 0;
    while (true) {
        std::cout << label;
        if (std::cin >> value && value >= min_val && value <= max_val) break;
        std::cout << "  Valor invalido. Ingresa un numero entre "
                  << min_val << " y " << max_val << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

static std::string prompt_path(const std::string& label, bool allow_empty = false)
{
    std::string path;
    while (true) {
        std::cout << label;
        std::getline(std::cin, path);
        if (allow_empty || !path.empty()) break;
        std::cout << "  La ruta no puede estar vacia.\n";
    }
    return path;
}

static uint32_t choose_block_size()
{
    std::cout << "\nTamano de bloque:\n"
              << "  1. 256 KB\n"
              << "  2. 1 MB  (recomendado)\n"
              << "  3. 4 MB\n";
    int choice = prompt_int("Opcion: ", 1, 3);
    switch (choice) {
        case 1: return 256  * 1024;
        case 3: return 4096 * 1024;
        default: return 1024 * 1024;
    }
}

static void print_separator()
{
    std::cout << "\n" << std::string(55, '-') << "\n";
}

// ── Compression flow ──────────────────────────────────────────────────────────

static void run_compression()
{
    print_separator();
    std::string input  = prompt_path("Archivo de entrada : ");
    std::string output = prompt_path("Archivo de salida  : ");
    int threads        = prompt_int("Numero de hilos    : ", 1, 256);
    uint32_t block_sz  = choose_block_size();

    std::cout << "\nComprimiendo...\n";
    CompressionResult res = compress_file(input, output, threads, block_sz);

    if (res.elapsed_seconds == 0.0 && res.original_bytes == 0) {
        std::cout << "La compresion fallo.\n";
        return;
    }

    double ratio = 100.0 * (1.0 - static_cast<double>(res.compressed_bytes)
                                  / static_cast<double>(res.original_bytes));

    print_separator();
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Tamano original    : " << res.original_bytes    << " bytes\n";
    std::cout << "Tamano comprimido  : " << res.compressed_bytes  << " bytes\n";
    std::cout << "Reduccion          : " << ratio                 << " %\n";
    std::cout << "Hilos              : " << threads               << "\n";
    std::cout << "Tamano de bloque   : " << block_sz / 1024       << " KB\n";
    std::cout << "Tiempo             : " << res.elapsed_seconds   << " s\n";
}

// ── Decompression flow ────────────────────────────────────────────────────────

static void run_decompression()
{
    print_separator();
    std::string input    = prompt_path("Archivo comprimido : ");
    std::string output   = prompt_path("Archivo de salida  : ");
    std::string original = prompt_path("Original (para verificar integridad, Enter para omitir): ", true);
    int threads          = prompt_int("Numero de hilos    : ", 1, 256);

    std::cout << "\nDescomprimiendo...\n";
    DecompressionResult res = decompress_file(input, output, threads, original);

    if (res.elapsed_seconds == 0.0 && res.decompressed_bytes == 0) {
        std::cout << "La descompresion fallo.\n";
        return;
    }

    print_separator();
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Bytes descomprimidos : " << res.decompressed_bytes << "\n";
    std::cout << "Hilos                : " << threads                << "\n";
    std::cout << "Tiempo               : " << res.elapsed_seconds    << " s\n";

    if (!original.empty()) {
        std::cout << "Integridad           : "
                  << (res.integrity_ok ? "OK - coincide con el original"
                                       : "FALLO - los archivos difieren")
                  << "\n";
    }
}

// ── Speedup benchmark ─────────────────────────────────────────────────────────

static void run_benchmark()
{
    print_separator();
    std::string input  = prompt_path("Archivo de entrada : ");
    std::string output = prompt_path("Archivo de salida  : ");
    uint32_t block_sz  = choose_block_size();

    const int thread_counts[] = { 1, 2, 4, 8 };
    double t_sequential = 0.0;

    print_separator();
    std::cout << std::left
              << std::setw(8)  << "Hilos"
              << std::setw(14) << "Tiempo (s)"
              << std::setw(10) << "Speedup"
              << "Eficiencia\n";
    std::cout << std::string(44, '-') << "\n";

    for (int n : thread_counts) {
        CompressionResult r = compress_file(input, output, n, block_sz);
        if (n == 1) t_sequential = r.elapsed_seconds;

        double speedup    = t_sequential / r.elapsed_seconds;
        double efficiency = speedup / static_cast<double>(n);

        std::cout << std::fixed << std::setprecision(4)
                  << std::setw(8)  << n
                  << std::setw(14) << r.elapsed_seconds
                  << std::setw(10) << speedup
                  << efficiency    << "\n";
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
    while (true) {
        print_separator();
        std::cout << "Compresion paralela con Pthreads + zlib\n";
        std::cout << "\n  1. Comprimir archivo"
                     "\n  2. Descomprimir archivo previamente comprimido"
                     "\n  3. Benchmark de speedup (1/2/4/8 hilos)"
                     "\n  0. Salir\n";

        int choice = prompt_int("\nOpcion: ", 0, 3);

        switch (choice) {
            case 1: run_compression();   break;
            case 2: run_decompression(); break;
            case 3: run_benchmark();     break;
            case 0:
                std::cout << "Hasta luego.\n";
                return 0;
        }
    }
}
