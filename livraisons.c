#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* ============================================================
   STRUCTURES
   ============================================================ */

typedef struct {
    int        N;
    long long  cap;
    long long **road;
    long long **air;
} Scenario;

typedef struct {
    int *truck_route;
    int  truck_size;
    int *drone1;
    int  d1_size;
    int *d1_takeoff;
    int *d1_land;
    int *drone2;
    int  d2_size;
    int *d2_takeoff;
    int *d2_land;
} Solution;

/* ============================================================
   ALLOCATION / LIBERATION
   ============================================================ */

Scenario alloc_scenario(int N) {
    Scenario s;
    s.N   = N;
    s.cap = 0;
    s.road = malloc((N + 1) * sizeof(long long *));
    s.air  = malloc((N + 1) * sizeof(long long *));
    for (int i = 0; i <= N; i++) {
        s.road[i] = malloc((N + 1) * sizeof(long long));
        s.air[i]  = malloc((N + 1) * sizeof(long long));
    }
    return s;
}

void free_scenario(Scenario *s) {
    for (int i = 0; i <= s->N; i++) {
        free(s->road[i]);
        free(s->air[i]);
    }
    free(s->road);
    free(s->air);
}

Solution alloc_solution(int N) {
    Solution sol;
    sol.truck_route = malloc(N * sizeof(int));
    sol.truck_size  = 0;
    sol.drone1      = malloc(N * sizeof(int));
    sol.d1_size     = 0;
    sol.d1_takeoff  = malloc(N * sizeof(int));
    sol.d1_land     = malloc(N * sizeof(int));
    sol.drone2      = malloc(N * sizeof(int));
    sol.d2_size     = 0;
    sol.d2_takeoff  = malloc(N * sizeof(int));
    sol.d2_land     = malloc(N * sizeof(int));
    return sol;
}

void free_solution(Solution *sol) {
    free(sol->truck_route);
    free(sol->drone1);  free(sol->d1_takeoff); free(sol->d1_land);
    free(sol->drone2);  free(sol->d2_takeoff); free(sol->d2_land);
}

Solution copier_solution(Solution *src, int N) {
    Solution dst = alloc_solution(N);
    dst.truck_size = src->truck_size;
    dst.d1_size    = src->d1_size;
    dst.d2_size    = src->d2_size;
    memcpy(dst.truck_route, src->truck_route, src->truck_size * sizeof(int));
    memcpy(dst.drone1,      src->drone1,      src->d1_size    * sizeof(int));
    memcpy(dst.d1_takeoff,  src->d1_takeoff,  src->d1_size    * sizeof(int));
    memcpy(dst.d1_land,     src->d1_land,     src->d1_size    * sizeof(int));
    memcpy(dst.drone2,      src->drone2,      src->d2_size    * sizeof(int));
    memcpy(dst.d2_takeoff,  src->d2_takeoff,  src->d2_size    * sizeof(int));
    memcpy(dst.d2_land,     src->d2_land,     src->d2_size    * sizeof(int));
    return dst;
}

/* ============================================================
   LECTURE
   ============================================================ */

Scenario lire_scenario(FILE *f) {
    int N;
    long long cap;
    fscanf(f, "%d %lld", &N, &cap);
    Scenario s = alloc_scenario(N);
    s.cap = cap;
    for (int i = 0; i <= N; i++)
        for (int j = 0; j <= N; j++)
            fscanf(f, "%lld", &s.road[i][j]);
    for (int i = 0; i <= N; i++)
        for (int j = 0; j <= N; j++)
            fscanf(f, "%lld", &s.air[i][j]);
    return s;
}

/* ============================================================
   TEMPS D'ARRIVEE DU CAMION
   arr[0] = 0 (depot), arr[i+1] = arrivee au truck_route[i]
   ============================================================ */

long long *calc_arrivals(Scenario *s, Solution *sol) {
    long long *arr = malloc((sol->truck_size + 1) * sizeof(long long));
    arr[0] = 0;
    for (int i = 0; i < sol->truck_size; i++) {
        int from = (i == 0) ? 0 : sol->truck_route[i - 1];
        int to   = sol->truck_route[i];
        arr[i + 1] = arr[i] + s->road[from][to];
    }
    return arr;
}

/* Heure d'arrivee du camion au site 'site' (doit etre dans la route) */
long long heure_arrivee(long long *arr, Solution *sol, int site) {
    if (site == 0) return 0;
    for (int i = 0; i < sol->truck_size; i++)
        if (sol->truck_route[i] == site)
            return arr[i + 1];
    return 0;
}

/* ============================================================
   CALCUL DU COUT TOTAL (somme des dates de livraison)
   ============================================================ */

long long calculer_cout(Scenario *s, Solution *sol) {
    long long somme = 0;
    long long *arr = calc_arrivals(s, sol);

    /* Livraisons camion */
    for (int i = 0; i < sol->truck_size; i++)
        somme += arr[i + 1];

    /* Livraisons drone 1 */
    for (int k = 0; k < sol->d1_size; k++) {
        int dep = sol->d1_takeoff[k];
        int cli = sol->drone1[k];
        long long t_dep = heure_arrivee(arr, sol, dep);
        somme += t_dep + s->air[dep][cli];
    }

    /* Livraisons drone 2 */
    for (int k = 0; k < sol->d2_size; k++) {
        int dep = sol->d2_takeoff[k];
        int cli = sol->drone2[k];
        long long t_dep = heure_arrivee(arr, sol, dep);
        somme += t_dep + s->air[dep][cli];
    }

    free(arr);
    return somme;
}

/* ============================================================
   VALIDATION VOL DRONE
   Verifie que le vol dep->cli->lan respecte la capacite vol.
   ============================================================ */

int vol_faisable(Scenario *s, Solution *sol, long long *arr,
                 int dep, int cli, int lan) {
    long long t_dep = heure_arrivee(arr, sol, dep);
    long long t_lan;

    /* si lan == 0, c'est le depot en fin de tournee */
    if (lan == 0) {
        /* camion rentre au depot apres le dernier client */
        if (sol->truck_size == 0) {
            t_lan = 0;
        } else {
            int last = sol->truck_route[sol->truck_size - 1];
            t_lan = arr[sol->truck_size] + s->road[last][0];
        }
    } else {
        t_lan = heure_arrivee(arr, sol, lan);
    }

    long long vol_aller  = s->air[dep][cli];
    long long vol_retour = s->air[cli][lan == 0 ? 0 : lan];
    long long t_drone_lan = t_dep + vol_aller + vol_retour;

    long long attente = (t_drone_lan < t_lan) ? (t_lan - t_drone_lan) : 0;
    long long total_air = vol_aller + vol_retour + attente;

    return total_air <= s->cap;
}

/* ============================================================
   PHASE 1 — CONSTRUCTION
   ============================================================ */

void construire_route_camion(Scenario *s, Solution *sol) {
    int *visite = calloc(s->N + 1, sizeof(int));
    sol->truck_size = 0;
    int current = 0;

    for (int step = 0; step < s->N; step++) {
        long long best = LLONG_MAX;
        int best_j = -1;
        for (int j = 1; j <= s->N; j++) {
            if (!visite[j] && s->road[current][j] < best) {
                best   = s->road[current][j];
                best_j = j;
            }
        }
        visite[best_j] = 1;
        sol->truck_route[sol->truck_size++] = best_j;
        current = best_j;
    }
    free(visite);
}

void affecter_drones(Scenario *s, Solution *sol) {
    int *marked = calloc(sol->truck_size, sizeof(int));
    long long *arr = calc_arrivals(s, sol);

    for (int i = 0; i < sol->truck_size; i++) {
        int cli = sol->truck_route[i];

        /* decollage : site precedent non marque, ou depot */
        int dep = 0;
        for (int d = i - 1; d >= 0; d--) {
            if (!marked[d]) { dep = sol->truck_route[d]; break; }
        }

        /* atterrissage : site suivant non marque, ou 0 (depot fin) */
        int lan = 0;
        for (int l = i + 1; l < sol->truck_size; l++) {
            if (!marked[l]) { lan = sol->truck_route[l]; break; }
        }

        /* choisir le drone le moins charge */
        int drone_cible = (sol->d1_size <= sol->d2_size) ? 1 : 2;

        if (vol_faisable(s, sol, arr, dep, cli, lan)) {
            marked[i] = 1;
            if (drone_cible == 1) {
                sol->drone1[sol->d1_size]     = cli;
                sol->d1_takeoff[sol->d1_size] = dep;
                sol->d1_land[sol->d1_size]    = lan;
                sol->d1_size++;
            } else {
                sol->drone2[sol->d2_size]     = cli;
                sol->d2_takeoff[sol->d2_size] = dep;
                sol->d2_land[sol->d2_size]    = lan;
                sol->d2_size++;
            }
        }
    }

    /* Reconstruire truck_route sans les clients affectes aux drones */
    int new_size = 0;
    int *new_route = malloc(sol->truck_size * sizeof(int));
    for (int i = 0; i < sol->truck_size; i++)
        if (!marked[i])
            new_route[new_size++] = sol->truck_route[i];
    memcpy(sol->truck_route, new_route, new_size * sizeof(int));
    sol->truck_size = new_size;

    free(new_route);
    free(marked);
    free(arr);
}

/* ============================================================
   PHASE 2 — AMELIORATION
   ============================================================ */

void inverser_segment(int *route, int i, int j) {
    while (i < j) {
        int tmp  = route[i];
        route[i] = route[j];
        route[j] = tmp;
        i++; j--;
    }
}

void deux_opt(Scenario *s, Solution *sol) {
    if (sol->truck_size < 2) return;
    int amelioration = 1;
    while (amelioration) {
        amelioration = 0;
        for (int i = 0; i < sol->truck_size - 1; i++) {
            for (int j = i + 1; j < sol->truck_size; j++) {
                long long avant = calculer_cout(s, sol);
                inverser_segment(sol->truck_route, i, j);
                long long apres = calculer_cout(s, sol);
                if (apres < avant) {
                    amelioration = 1;
                } else {
                    inverser_segment(sol->truck_route, i, j);
                }
            }
        }
    }
}

void reaffecter_camion_vers_drone(Scenario *s, Solution *sol) {
    int amelioration = 1;
    while (amelioration) {
        amelioration = 0;
        long long *arr = calc_arrivals(s, sol);

        for (int i = 0; i < sol->truck_size; i++) {
            int cli = sol->truck_route[i];
            int dep = (i == 0) ? 0 : sol->truck_route[i - 1];
            int lan = (i == sol->truck_size - 1) ? 0 : sol->truck_route[i + 1];
            int drone_cible = (sol->d1_size <= sol->d2_size) ? 1 : 2;

            if (!vol_faisable(s, sol, arr, dep, cli, lan)) continue;

            long long avant = calculer_cout(s, sol);
            Solution tmp = copier_solution(sol, s->N);

            /* retirer cli de la route camion */
            for (int k = i; k < tmp.truck_size - 1; k++)
                tmp.truck_route[k] = tmp.truck_route[k + 1];
            tmp.truck_size--;

            if (drone_cible == 1) {
                tmp.drone1[tmp.d1_size]     = cli;
                tmp.d1_takeoff[tmp.d1_size] = dep;
                tmp.d1_land[tmp.d1_size]    = lan;
                tmp.d1_size++;
            } else {
                tmp.drone2[tmp.d2_size]     = cli;
                tmp.d2_takeoff[tmp.d2_size] = dep;
                tmp.d2_land[tmp.d2_size]    = lan;
                tmp.d2_size++;
            }

            long long apres = calculer_cout(s, &tmp);
            if (apres < avant) {
                free_solution(sol);
                *sol = tmp;
                amelioration = 1;
                free(arr);
                arr = calc_arrivals(s, sol);
            } else {
                free_solution(&tmp);
            }
        }
        free(arr);
    }
}

/* ============================================================
   ECRITURE DE LA SOLUTION
   ============================================================ */

void ecrire_solution(Scenario *s, Solution *sol) {
    printf("%d %d %d\n", sol->truck_size, sol->d1_size, sol->d2_size);

    for (int i = 0; i < sol->truck_size; i++)
        printf("%d ", sol->truck_route[i]);
    printf("\n");

    for (int i = 0; i < sol->d1_size; i++) printf("%d ", sol->drone1[i]);
    printf(". ");
    for (int i = 0; i < sol->d2_size; i++) printf("%d ", sol->drone2[i]);
    printf("\n");

    for (int i = 0; i < sol->d1_size; i++) printf("%d ", sol->d1_takeoff[i]);
    printf(". ");
    for (int i = 0; i < sol->d2_size; i++) printf("%d ", sol->d2_takeoff[i]);
    printf("\n");

    /* atterrissage = 0 signifie fin de tournee -> afficher N+1 */
    for (int i = 0; i < sol->d1_size; i++) {
        int v = sol->d1_land[i];
        printf("%d ", (v == 0) ? s->N + 1 : v);
    }
    printf(". ");
    for (int i = 0; i < sol->d2_size; i++) {
        int v = sol->d2_land[i];
        printf("%d ", (v == 0) ? s->N + 1 : v);
    }
    printf("\n");
}

/* ============================================================
   MAIN
   ============================================================ */

int main(void) {
    int t;
    scanf("%d", &t);

    for (int scenario = 0; scenario < t; scenario++) {
        Scenario s  = lire_scenario(stdin);
        Solution sol = alloc_solution(s.N);

        /* Phase 1 : construction */
        construire_route_camion(&s, &sol);
        affecter_drones(&s, &sol);

        /* Phase 2 : amelioration */
        deux_opt(&s, &sol);
        reaffecter_camion_vers_drone(&s, &sol);

        /* Sortie */
        ecrire_solution(&s, &sol);

        free_solution(&sol);
        free_scenario(&s);
    }

    return 0;
}