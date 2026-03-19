#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

struct Context
{
  int N;           // Le nombre de clients (sans le dépôt)
  int cap;         // La capacité des drones
  int** road_mat;  // Matrice des temps de trajet par la route
  int** fly_mat;   // Matrice des temps de trajet par les airs
};

struct DroneDelivery
{
  int size;
  int* clients;
  int* takeoffs;
  int* landings;
};

struct TruckDelivery
{
  int size;
  int* clients;
};

struct Solution
{
  struct TruckDelivery truck;
  struct DroneDelivery *drones;
};

// Helper function to find the maximum of two integers
int max(int a, int b) {
    return (a > b) ? a : b;
}

struct Solution solve_and_return(struct Context context){
  int N = context.N;
  int cap = context.cap;
  bool* visited = calloc(N + 1, sizeof(bool));
  visited[0] = true; // Depot is visited

  // Temporary storage for truck route and drone deliveries
  int* truck_temp_clients = malloc(N * sizeof(int));
  int truck_temp_count = 0;

  // Drone 0
  int* d0_clients = malloc(N * sizeof(int));
  int* d0_takeoffs = malloc(N * sizeof(int));
  int* d0_landings = malloc(N * sizeof(int));
  int d0_count = 0;

  // Drone 1
  int* d1_clients = malloc(N * sizeof(int));
  int* d1_takeoffs = malloc(N * sizeof(int));
  int* d1_landings = malloc(N * sizeof(int));
  int d1_count = 0;

  int current_truck_pos = 0;
  int current_truck_time = 0;

  // Keep track of truck's path and arrival times at each stop
  // This is crucial for drone coordination
  int* truck_path_nodes = malloc((N + 1) * sizeof(int));
  int* truck_path_times = malloc((N + 1) * sizeof(int));
  int truck_path_len = 0;

  truck_path_nodes[truck_path_len] = 0;
  truck_path_times[truck_path_len] = 0;
  truck_path_len++;

  while (truck_temp_count < N) {
    // At each truck stop, try to dispatch drones
    // Drones can take off from current_truck_pos and land at a future truck_stop

    // Try to find clients for drones from the current truck position
    // Prioritize clients that can be served by drone and return to a future truck stop

    // For simplicity in this greedy approach, let's assume drones return to the *current* truck position
    // This is a limitation that can be improved with more complex logic

    // Find best drone 0 candidate
    int best_d0_client = -1;
    int min_d0_total_flight_time = INT_MAX;

    for (int client_idx = 1; client_idx <= N; client_idx++) {
      if (!visited[client_idx]) {
        int flight_time_there = context.fly_mat[current_truck_pos][client_idx];
        int flight_time_back = context.fly_mat[client_idx][current_truck_pos];
        int total_flight_time = flight_time_there + flight_time_back;

        if (total_flight_time <= cap) {
          if (total_flight_time < min_d0_total_flight_time) {
            min_d0_total_flight_time = total_flight_time;
            best_d0_client = client_idx;
          }
        }
      }
    }

    // Find best drone 1 candidate (different from drone 0's client)
    int best_d1_client = -1;
    int min_d1_total_flight_time = INT_MAX;

    for (int client_idx = 1; client_idx <= N; client_idx++) {
      if (!visited[client_idx] && client_idx != best_d0_client) {
        int flight_time_there = context.fly_mat[current_truck_pos][client_idx];
        int flight_time_back = context.fly_mat[client_idx][current_truck_pos];
        int total_flight_time = flight_time_there + flight_time_back;

        if (total_flight_time <= cap) {
          if (total_flight_time < min_d1_total_flight_time) {
            min_d1_total_flight_time = total_flight_time;
            best_d1_client = client_idx;
          }
        }
      }
    }

    // Assign drones if candidates found
    if (best_d0_client != -1) {
      visited[best_d0_client] = true;
      d0_clients[d0_count] = best_d0_client;
      d0_takeoffs[d0_count] = current_truck_pos;
      d0_landings[d0_count] = current_truck_pos; // Drone returns to current truck pos
      d0_count++;
    }
    if (best_d1_client != -1) {
      visited[best_d1_client] = true;
      d1_clients[d1_count] = best_d1_client;
      d1_takeoffs[d1_count] = current_truck_pos;
      d1_landings[d1_count] = current_truck_pos; // Drone returns to current truck pos
      d1_count++;
    }

    // Find the nearest unvisited client for the truck
    int nearest_truck_client = -1;
    int min_road_dist = INT_MAX;

    for (int client_idx = 1; client_idx <= N; client_idx++) {
      if (!visited[client_idx]) {
        int dist = context.road_mat[current_truck_pos][client_idx];
        if (dist < min_road_dist) {
          min_road_dist = dist;
          nearest_truck_client = client_idx;
        }
      }
    }

    if (nearest_truck_client != -1) {
      truck_temp_clients[truck_temp_count++] = nearest_truck_client;
      visited[nearest_truck_client] = true;
      current_truck_time += min_road_dist;
      current_truck_pos = nearest_truck_client;

      truck_path_nodes[truck_path_len] = current_truck_pos;
      truck_path_times[truck_path_len] = current_truck_time;
      truck_path_len++;

    } else {
      // No more unvisited clients for the truck, break the loop
      break;
    }
  }

  free(visited);

  // Finalize truck delivery
  struct TruckDelivery truck;
  truck.size = truck_temp_count;
  truck.clients = malloc(truck_temp_count * sizeof(int));
  for (int i = 0; i < truck_temp_count; i++) truck.clients[i] = truck_temp_clients[i];
  free(truck_temp_clients);

  // Finalize drone deliveries
  struct DroneDelivery *drones = malloc(2 * sizeof(struct DroneDelivery));

  drones[0].size = d0_count;
  drones[0].clients = malloc(d0_count * sizeof(int));
  drones[0].takeoffs = malloc(d0_count * sizeof(int));
  drones[0].landings = malloc(d0_count * sizeof(int));
  for (int i = 0; i < d0_count; i++) {
    drones[0].clients[i] = d0_clients[i];
    drones[0].takeoffs[i] = d0_takeoffs[i];
    drones[0].landings[i] = d0_landings[i];
  }

  drones[1].size = d1_count;
  drones[1].clients = malloc(d1_count * sizeof(int));
  drones[1].takeoffs = malloc(d1_count * sizeof(int));
  drones[1].landings = malloc(d1_count * sizeof(int));
  for (int i = 0; i < d1_count; i++) {
    drones[1].clients[i] = d1_clients[i];
    drones[1].takeoffs[i] = d1_takeoffs[i];
    drones[1].landings[i] = d1_landings[i];
  }

  free(d0_clients); free(d0_takeoffs); free(d0_landings);
  free(d1_clients); free(d1_takeoffs); free(d1_landings);
  free(truck_path_nodes); free(truck_path_times);

  struct Solution sol = {truck, drones};
  return sol;
}

int** read_matrix(int size, FILE* file) {
  int i,j;
  int **mat = malloc(size*sizeof(int*));
  for(i=0;i<size;i++){
    mat[i] = malloc(size*sizeof(int));
    for(j=0;j<size;j++){
      if (fscanf(file,"%d", &mat[i][j]) != 1) return NULL;
    }
  }
  return mat;
}

void clear_mat(int** mat, int size){
  int i;
  for (i = 0; i<size; i++) free(mat[i]);
  free(mat);
}

struct Context read_context(FILE* file) {
  int N, cap;
  if (fscanf(file, "%d %d", &N, &cap) != 2) {
      struct Context empty = {0, 0, NULL, NULL};
      return empty;
  }
  int **road_mat = read_matrix(N+1,file);
  int **fly_mat = read_matrix(N+1,file);
  struct Context context = {N, cap, road_mat, fly_mat};
  return context;  
}

void clear_context(struct Context* context) {
  clear_mat(context->road_mat, context->N + 1);
  clear_mat(context->fly_mat, context->N + 1);
}

void clear_solution(struct Solution* solution){
  free(solution->truck.clients);
  for (int i=0; i<2; i++){
    free(solution->drones[i].clients);
    free(solution->drones[i].takeoffs);
    free(solution->drones[i].landings);
  }
  free(solution->drones);
}

void print_solution(struct Solution *solution, FILE* file){
  struct TruckDelivery truck = solution->truck;
  struct DroneDelivery *drones  = solution->drones;
  fprintf(file, "%d %d %d\n", truck.size, drones[0].size, drones[1].size);
  for (int i = 0; i < truck.size; i++) fprintf(file, "%d ", truck.clients[i]);
  fprintf(file, "\n");
  for (int i = 0; i < drones[0].size; i++) fprintf(file, "%d ", drones[0].clients[i]);
  fprintf(file, ". ");
  for (int i = 0; i < drones[1].size; i++) fprintf(file, "%d ", drones[1].clients[i]);
  fprintf(file, "\n");
  for (int i = 0; i < drones[0].size; i++) fprintf(file, "%d ", drones[0].takeoffs[i]);
  fprintf(file, ". ");
  for (int i = 0; i < drones[1].size; i++) fprintf(file, "%d ", drones[1].takeoffs[i]);
  fprintf(file, "\n");
  for (int i = 0; i < drones[0].size; i++) fprintf(file, "%d ", drones[0].landings[i]);
  fprintf(file, ". ");
  for (int i = 0; i < drones[1].size; i++) fprintf(file, "%d ", drones[1].landings[i]);
  fprintf(file, "\n"); 
}

int main(void){
  int t, i;
  if (fscanf(stdin, "%d", &t) != 1) return 0;
  for (i = 0; i < t; i++) {
    struct Context context = read_context(stdin);
    if (context.road_mat == NULL) break;
    struct Solution solution = solve_and_return(context);
    print_solution(&solution, stdout);
    clear_context(&context);
    clear_solution(&solution);
  }
  return 0;
}
