#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>



#define HT_SIZE 262147   /* primo grande para menos colisiones */

typedef struct ht_node {
    uint64_t key;       // vpage (64 bits)
    int value;          // frame index
    struct ht_node *next;
} ht_node_t;

static ht_node_t **ht_buckets = NULL;

static void ht_init(void) {
    ht_buckets = calloc(HT_SIZE, sizeof(ht_node_t*));
    if (!ht_buckets) { perror("calloc"); exit(1); }
}

static inline size_t ht_hash(uint64_t k) {
    return (size_t)((k * 11400714819323198485ULL) % HT_SIZE);
}

static int ht_get(uint64_t key) {
    size_t h = ht_hash(key);
    ht_node_t *n = ht_buckets[h];
    while (n) {
        if (n->key == key) return n->value;
        n = n->next;
    }
    return -1;
}

static void ht_put(uint64_t key, int value) {
    size_t h = ht_hash(key);
    ht_node_t *n = ht_buckets[h];

    while (n) {
        if (n->key == key) { n->value = value; return; }
        n = n->next;
    }

    ht_node_t *nn = malloc(sizeof(ht_node_t));
    if (!nn) { perror("malloc"); exit(1); }
    nn->key = key;
    nn->value = value;
    nn->next = ht_buckets[h];
    ht_buckets[h] = nn;
}

static void ht_remove(uint64_t key) {
    size_t h = ht_hash(key);
    ht_node_t *n = ht_buckets[h], *prev = NULL;

    while (n) {
        if (n->key == key) {
            if (prev) prev->next = n->next;
            else ht_buckets[h] = n->next;
            free(n);
            return;
        }
        prev = n;
        n = n->next;
    }
}

static void ht_free(void) {
    for (size_t i = 0; i < HT_SIZE; i++) {
        ht_node_t *n = ht_buckets[i];
        while (n) {
            ht_node_t *tmp = n->next;
            free(n);
            n = tmp;
        }
    }
    free(ht_buckets);
}

/* ----------------------- Helper: verificar potencia de 2 ----------------------- */

static int pow2_log2(uint64_t x) {
    if (x == 0) return -1;
    if ((x & (x - 1)) != 0) return -1; // no es potencia de 2

    int b = 0;
    while ((1ULL << b) < x) b++;
    return b;
}


static void usage(const char *p) {
    fprintf(stderr,
        "Uso: %s Nmarcos tamañomarco [--verbose] traza.txt\n"
        "  Nmarcos: número de marcos físicos\n"
        "  tamañomarco: potencia de 2 (ej: 8, 4096)\n",
        p);
}


int main(int argc, char *argv[]) {
    if (argc < 4) { usage(argv[0]); return 1; }

    int argi = 1;

    long Nmarcos = strtol(argv[argi++], NULL, 10);
    if (Nmarcos <= 0) { fprintf(stderr, "Nmarcos debe ser >0\n"); return 1; }

    uint64_t frame_size = strtoull(argv[argi++], NULL, 10);
    int b = pow2_log2(frame_size);
    if (b < 0) {
        fprintf(stderr, "tamañomarco debe ser potencia de 2.\n");
        return 1;
    }

    uint64_t MASK = frame_size - 1;

    int verbose = 0;
    char *tracefile = NULL;

    while (argi < argc) {
        if (!strcmp(argv[argi], "--verbose")) {
            verbose = 1;
            argi++;
        } else {
            tracefile = argv[argi++];
        }
    }

    if (!tracefile) {
        fprintf(stderr, "Debe especificar archivo de traza\n");
        return 1;
    }

    FILE *f = fopen(tracefile, "r");
    if (!f) { perror("fopen traza"); return 1; }

    ht_init();

    int nframes = (int)Nmarcos;
    uint64_t *frame_vpage = calloc(nframes, sizeof(uint64_t));
    //int *frame_vpage = calloc(nframes, sizeof(int));
    uint8_t *refbit = calloc(nframes, sizeof(uint8_t));
    if (!frame_vpage || !refbit) { perror("calloc"); exit(1); }

    for (int i = 0; i < nframes; i++) frame_vpage[i] = -1;

    int free_frames = nframes;
    int clock_ptr = 0;

    uint64_t refs = 0;
    uint64_t faults = 0;

    char line[256];

    while (fgets(line, sizeof(line), f)) {
        char *s = line;

        while (isspace((unsigned char)*s)) s++;
        if (*s == '\0' || *s == '#') continue;

        char *end = s + strlen(s) - 1;
        while (end > s && isspace((unsigned char)*end)) *end-- = '\0';

        errno = 0;
        uint64_t VA = strtoull(s, NULL, 0);
        if (errno) continue;

        refs++;

        uint64_t offset = VA & MASK;
        uint64_t nvp = VA >> b;

        int frame = ht_get(nvp);

        if (frame >= 0) {
            refbit[frame] = 1;
            uint64_t DF = ((uint64_t)frame << b) | offset;

            if (verbose) {
                printf("DV=0x%016" PRIx64 " nvp=%" PRIu64 " offset=%" PRIu64
                       " HIT mar=%d DF=0x%016" PRIx64 "\n",
                       VA, nvp, offset, frame, DF);
            }
        } else {
            faults++;

            int used = -1;

            if (free_frames > 0) {
                for (int i = 0; i < nframes; i++) {
                    if (frame_vpage[i] == -1) { used = i; break; }
                }
                free_frames--;
            } else {
                while (1) {
                    if (refbit[clock_ptr] == 0) {
                        used = clock_ptr;
                        break;
                    } else {
                        refbit[clock_ptr] = 0;
                        clock_ptr = (clock_ptr + 1) % nframes;
                    }
                }

                uint64_t victim = frame_vpage[used];
                if (victim != (uint64_t)-1) ht_remove(victim);
            }

            
            frame_vpage[used] = nvp;
            refbit[used] = 1;
            ht_put(nvp, used);

            uint64_t DF = ((uint64_t)used << b) | offset;

            if (verbose) {
                printf("DV=0x%016" PRIx64 " nvp=%" PRIu64 " offset=%" PRIu64
                       " FALLO mar=%d DF=0x%016" PRIx64 "\n",
                       VA, nvp, offset, used, DF);
            }

            clock_ptr = (used + 1) % nframes;
        }
    }

    fclose(f);

    double tasa = refs ? (double)faults / (double)refs : 0.0;

    printf("Totales:\n");
    printf("  Referencias: %" PRIu64 "\n", refs);
    printf("  Fallos de página: %" PRIu64 "\n", faults);
    printf("  Tasa de fallos: %.6f\n", tasa);

    free(frame_vpage);
    free(refbit);
    ht_free();

    return 0;
}
