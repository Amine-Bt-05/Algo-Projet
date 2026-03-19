#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Context
{
  int N;           // Le nombre de clients (sans le dépôt)
  int cap;         // La capacité des drones
  int** road_mat;  // Matrice des temps de trajet par la route
  int** fly_mat;   // Matrice des temps de trajet par les airs
};

struct DroneDelivery  // Structure d'une tournée pour un drone (nb de clients servis puis trois tableaux ordonnées)
{
  int size;        // Nombre de clients servis
  int* clients;    // Liste des clients dans l'ordre
  int* takeoffs;   // Liste des décollages dans l'ordre  (noter N+1 si noeud 0 en fin de parcours)
  int* landings;   // Liste des atterissages dans l'ordre (noter N+1 si noeud 0 en fin de parcours)
};

struct TruckDelivery // Structure tournée par la route (la taille et la séquence de clients)
{
  int size;
  int* clients;
};

struct Solution    // Structure de solution (une tournée par la route et un tableau de tournées de drones
{
  struct TruckDelivery truck;
  struct DroneDelivery *drones;
};

struct Solution solve_and_return(struct Context context) {
    int N = context.N;
    int cap = context.cap;
    int **road_mat = context.road_mat;
    int **fly_mat = context.fly_mat;

    char *visited = calloc(N + 1, sizeof(char));
    int *truck_clients = malloc(N * sizeof(int));
    int truck_size = 0;

    int *d1_clients = malloc(N * sizeof(int));
    int *d1_takeoffs = malloc(N * sizeof(int));
    int *d1_landings = malloc(N * sizeof(int));
    int d1_size = 0;

    int *d2_clients = malloc(N * sizeof(int));
    int *d2_takeoffs = malloc(N * sizeof(int));
    int *d2_landings = malloc(N * sizeof(int));
    int d2_size = 0;

    int current_pos = 0;
    visited[0] = 1;

    while (1) {
        int best_next = -1;
        int min_dist = 2000000000;

        for (int i = 1; i <= N; i++) {
            if (!visited[i]) {
                if (road_mat[current_pos][i] < min_dist) {
                    min_dist = road_mat[current_pos][i];
                    best_next = i;
                }
            }
        }

        if (best_next == -1) break;

        // Try to see if we can deliver a nearby neighbor by drone
        int drone_client = -1;
        int min_drone_dist = 2000000000;
        for (int i = 1; i <= N; i++) {
            if (!visited[i] && i != best_next) {
                // Drone flies current_pos -> i -> best_next
                // Wait time is max(0, truck_time - drone_time)
                // Total flight time = fly_mat[current_pos][i] + fly_mat[i][best_next] + wait
                int flight_dist = fly_mat[current_pos][i] + fly_mat[i][best_next];
                int truck_dist = road_mat[current_pos][best_next];
                int total_flight_time = (flight_dist > truck_dist) ? flight_dist : truck_dist;

                if (total_flight_time <= cap) {
                    if (flight_dist < min_drone_dist) {
                        min_drone_dist = flight_dist;
                        drone_client = i;
                    }
                }
            }
        }

        // Add truck client
        truck_clients[truck_size++] = best_next;
        visited[best_next] = 1;
        
        if (drone_client != -1) {
            if (d1_size <= d2_size) {
                d1_clients[d1_size] = drone_client;
                d1_takeoffs[d1_size] = current_pos;
                d1_landings[d1_size] = best_next;
                d1_size++;
            } else {
                d2_clients[d2_size] = drone_client;
                d2_takeoffs[d2_size] = current_pos;
                d2_landings[d2_size] = best_next;
                d2_size++;
            }
            visited[drone_client] = 1;
        }
        
        current_pos = best_next;
    }

    free(visited);

    struct TruckDelivery truck = {truck_size, truck_clients};
    struct DroneDelivery d1 = {d1_size, d1_clients, d1_takeoffs, d1_landings};
    struct DroneDelivery d2 = {d2_size, d2_clients, d2_takeoffs, d2_landings};

    struct DroneDelivery *drones = malloc(2 * sizeof(struct DroneDelivery));
    drones[0] = d1;
    drones[1] = d2;

    struct Solution sol = {truck, drones};
    return sol;
}

int** read_matrix(int size, FILE* file) 
{
  int i,j;
  int **mat = malloc(size*sizeof(int*));
  for(i=0;i<size;i++){
    mat[i] = malloc(size*sizeof(int));
    for(j=0;j<size;j++){
      fscanf(file,"%d", &mat[i][j]);
    }
  }
  return mat;
}

void clear_mat(int** mat, int size){
  int i;
  for (i = 0; i<size; i++){
    free(mat[i]);
  }
  free(mat);
}

struct Context read_context(FILE* file)
{
  int N, cap;
  if (fscanf(file, "%d %d\n", &N, &cap) != 2) return (struct Context){0,0,NULL,NULL};

  int **road_mat = read_matrix(N+1,file);
  int **fly_mat = read_matrix(N+1,file);

  struct Context context = {N, cap, road_mat, fly_mat};
  return context;  
}

void clear_context(struct Context* context)
{
  clear_mat(context->road_mat, context->N + 1);
  clear_mat(context->fly_mat, context->N + 1);
}

void clear_solution(struct Solution* solution){
  struct TruckDelivery truck = solution->truck;
  free(truck.clients);
  struct DroneDelivery drone;
  int i;
  for (i=0; i<2; i++){
    drone = solution->drones[i];
    free(drone.clients);
    free(drone.takeoffs);
    free(drone.landings);
  }
  free(solution->drones);
}

void print_solution(struct Solution *solution, FILE* file){
  struct TruckDelivery truck = solution->truck;
  struct DroneDelivery *drones  = solution->drones;
  int a = truck.size;
  int b = drones[0].size;
  int c = drones[1].size;
  int i;
  fprintf(file, "%d %d %d\n",a,b,c);
  for (i = 0; i < a; i++){
    fprintf(file, "%d ", truck.clients[i]);
  }
  fprintf(file, "\n");
  for (i = 0; i < b; i++){
    fprintf(file, "%d ", drones[0].clients[i]);
  }
  fprintf(file, ". ");
  for (i = 0; i < c; i++){
    fprintf(file, "%d ", drones[1].clients[i]);
  }
  fprintf(file, "\n");
  for (i = 0; i < b; i++){
    fprintf(file, "%d ", drones[0].takeoffs[i]);
  }
  fprintf(file, ". ");
  for (i = 0; i < c; i++){
    fprintf(file, "%d ", drones[1].takeoffs[i]);
  }
  fprintf(file, "\n");
  for (i = 0; i < b; i++){
    fprintf(file, "%d ", drones[0].landings[i]);
  }
  fprintf(file, ". ");
  for (i = 0; i < c; i++){
    fprintf(file, "%d ", drones[1].landings[i]);
  }
  fprintf(file, "\n"); 
}

int main(void){
  int t, i;
  struct Context context;
  struct Solution solution;
  if (fscanf(stdin, "%d", &t) != 1) return 0;
  for (i = 0; i < t; i++)
    {
      context = read_context(stdin);
      if (context.N == 0) break;
      solution = solve_and_return(context);
      print_solution(&solution, stdout);
      clear_context(&context);
      clear_solution(&solution);
    }
  return 0;
}
