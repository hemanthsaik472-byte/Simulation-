#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define INF 1e30
#define MAX_QUEUE 100000

/* -------- Random number helpers -------- */

// Uniform random in (0,1)
double uniform_rand() {
    return (rand() + 1.0) / (RAND_MAX + 2.0);
}

// Exponential random variable with given rate (lambda)
double exp_rand(double rate) {
    return -log(uniform_rand()) / rate;
}

/* -------- Simple circular queue to store arrival times -------- */

typedef struct {
    double data[MAX_QUEUE];
    int front;
    int rear;
    int count;
} Queue;

void init_queue(Queue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

int is_empty(Queue *q) {
    return q->count == 0;
}

int is_full(Queue *q) {
    return q->count == MAX_QUEUE;
}

void enqueue(Queue *q, double value) {
    if (is_full(q)) {
        printf("Queue overflow! Increase MAX_QUEUE.\n");
        exit(1);
    }
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->count++;
}

double dequeue(Queue *q) {
    if (is_empty(q)) {
        printf("Queue underflow! Something went wrong.\n");
        exit(1);
    }
    double value = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->count--;
    return value;
}

/* -------- Main simulation -------- */

int main() {
    double arrival_rate, service_rate, total_sim_time;

    printf("=== Single Server Queue Simulation (Bank/ATM) ===\n");
    printf("Enter arrival rate  (customers per time unit, e.g. 0.5): ");
    scanf("%lf", &arrival_rate);

    printf("Enter service rate  (customers per time unit, e.g. 0.7): ");
    scanf("%lf", &service_rate);

    printf("Enter total simulation time (e.g. 1000): ");
    scanf("%lf", &total_sim_time);

    if (arrival_rate <= 0 || service_rate <= 0 || total_sim_time <= 0) {
        printf("All values must be positive.\n");
        return 1;
    }

    // Seed random generator
    srand((unsigned int)time(NULL));

    // State variables
    double clock_time = 0.0;
    double next_arrival = exp_rand(arrival_rate);
    double next_departure = INF;

    int server_busy = 0;
    int queue_length = 0;
    int max_queue_length = 0;

    Queue q;
    init_queue(&q);

    // Statistics
    double last_event_time = 0.0;
    double area_num_in_queue = 0.0; // For average queue length
    double busy_time = 0.0;         // For utilization
    double total_wait_time = 0.0;   // Sum of waits for all customers
    int customers_served = 0;

    // Simulation loop
    while (clock_time < total_sim_time) {

        // Decide next event: arrival or departure
        if (next_arrival <= next_departure && next_arrival <= total_sim_time) {
            /* ---- Process arrival ---- */
            double time_advance = next_arrival - clock_time;

            // Update time-based stats
            area_num_in_queue += queue_length * time_advance;
            if (server_busy) busy_time += time_advance;

            clock_time = next_arrival;
            last_event_time = clock_time;

            // Check server status
            if (!server_busy) {
                // Server idle -> customer starts service immediately
                server_busy = 1;
                double service_time = exp_rand(service_rate);
                next_departure = clock_time + service_time;
                // wait time for this customer is 0, but adding 0 not needed
            } else {
                // Server busy -> customer joins queue
                enqueue(&q, clock_time);
                queue_length++;
                if (queue_length > max_queue_length) {
                    max_queue_length = queue_length;
                }
            }

            // Schedule next arrival
            next_arrival = clock_time + exp_rand(arrival_rate);

        } else {
            /* ---- Process departure ---- */
            if (next_departure == INF) {
                // No departure scheduled and arrival is after sim end
                break;
            }

            if (next_departure > total_sim_time) {
                // Next departure occurs after simulation end
                double time_advance = total_sim_time - clock_time;
                area_num_in_queue += queue_length * time_advance;
                if (server_busy) busy_time += time_advance;
                clock_time = total_sim_time;
                break;
            }

            double time_advance = next_departure - clock_time;
            area_num_in_queue += queue_length * time_advance;
            if (server_busy) busy_time += time_advance;

            clock_time = next_departure;
            last_event_time = clock_time;

            customers_served++;

            if (queue_length > 0) {
                // Take next customer from queue
                double arrival_time = dequeue(&q);
                queue_length--;

                double wait = clock_time - arrival_time;
                total_wait_time += wait;

                // Start service for this customer
                double service_time = exp_rand(service_rate);
                next_departure = clock_time + service_time;
            } else {
                // Queue empty -> server becomes idle
                server_busy = 0;
                next_departure = INF;
            }
        }
    }

    // Final statistics
    double sim_duration = clock_time;  // actual simulated time
    double avg_wait_time = (customers_served > 0)
                           ? (total_wait_time / customers_served)
                           : 0.0;
    double avg_queue_length = area_num_in_queue / sim_duration;
    double utilization = busy_time / sim_duration;
    double throughput = (customers_served) / sim_duration;

    printf("\n=== Simulation Results ===\n");
    printf("Total simulated time        : %.2f\n", sim_duration);
    printf("Customers served            : %d\n", customers_served);
    printf("Maximum queue length        : %d\n", max_queue_length);
    printf("Average waiting time        : %.4f time units\n", avg_wait_time);
    printf("Average number in queue     : %.4f\n", avg_queue_length);
    printf("Server utilization          : %.4f\n", utilization);
    printf("Throughput (cust/time unit) : %.4f\n", throughput);

    if (arrival_rate >= service_rate) {
        printf("\n[Note] Arrival rate >= service rate. ");
        printf("The system may be unstable (queue tends to grow).\n");
    }

    return 0;
}
