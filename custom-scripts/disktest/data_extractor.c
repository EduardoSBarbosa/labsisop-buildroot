#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s <arquivo_log> <escalonador> <metodo> <workers>\n", argv[0]);
        printf("Ex: %s test_sstf_sequential_10.txt sstf Sequential 10\n", argv[0]);
        return 1;
    }

    char *logfile = argv[1];
    char *scheduler = argv[2];
    char *method = argv[3];
    char *workers = argv[4];

    FILE *file = fopen(logfile, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo de log");
        return 1;
    }

    unsigned long long total_seek = 0;
    unsigned long long total_time_ns = 0;
    unsigned long long max_time_ns = 0;
    unsigned long long req_count = 0;

    char line[1024];

    // Varredura do arquivo linha por linha
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "[OUT]")) {
            char *seek_ptr = strstr(line, "Seek distance: ");
            // CORREÇÃO: Removido os ':' após o ElapsedTime para bater com o log
            char *time_ptr = strstr(line, "ElapsedTime "); 

            if (seek_ptr && time_ptr) {
                unsigned long long current_seek = 0;
                unsigned long long current_time = 0;

                sscanf(seek_ptr, "Seek distance: %llu", &current_seek);
                // CORREÇÃO: Removido os ':' no sscanf também
                sscanf(time_ptr, "ElapsedTime %llu", &current_time);

                total_seek += current_seek;
                total_time_ns += current_time;
                req_count++;

                if (current_time > max_time_ns) {
                    max_time_ns = current_time;
                }
            }
        }
    }

    fclose(file);

    if (req_count == 0) {
        printf("Nenhuma métrica encontrada em %s\n", logfile);
        return 0;
    }

    unsigned long long avg_time_ns = total_time_ns / req_count;

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char csv_filename[128];
    snprintf(csv_filename, sizeof(csv_filename), "%s_relatorio_%04d-%02d-%02d.csv", 
             scheduler, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    FILE *csv = fopen(csv_filename, "a");
    if (!csv) {
        perror("Erro ao criar CSV");
        return 1;
    }

    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0) {
        fprintf(csv, "Escalonador,Metodo,Workers,Total_Reqs,Total_Seek,Tempo_Medio_ns,Tempo_Maximo_ns\n");
    }

    fprintf(csv, "%s,%s,%s,%llu,%llu,%llu,%llu\n", 
            scheduler, method, workers, req_count, total_seek, avg_time_ns, max_time_ns);

    fclose(csv);
    printf("Dados salvos no relatorio dedicado: %s\n", csv_filename);

    return 0;
}