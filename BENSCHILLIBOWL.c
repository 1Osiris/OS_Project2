#include "BENSCHILLIBOWL.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

MenuItem PickRandomMenuItem() {
    return BENSCHILLIBOWLMenu[rand() % BENSCHILLIBOWLMenuLength];
}

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    
    bcb->orders = (Order**) malloc(sizeof(Order*) * max_size);
    bcb->max_size = max_size;
    bcb->expected_num_orders = expected_num_orders;
    bcb->num_orders_handled = 0;
    bcb->num_orders_received = 0;
    bcb->head = 0;
    bcb->tail = 0;
    
    // Initialize synchronization primitives
    pthread_mutex_init(&(bcb->mutex), NULL);
    pthread_cond_init(&(bcb->can_add_orders), NULL);
    pthread_cond_init(&(bcb->can_get_orders), NULL);
    
    printf("Restaurant is open!\n");
    return bcb;
}

void CloseRestaurant(BENSCHILLIBOWL* bcb) {
    // Wait until all orders have been handled
    pthread_mutex_lock(&(bcb->mutex));
    while (bcb->num_orders_handled != bcb->expected_num_orders) {
        pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
    }
    pthread_mutex_unlock(&(bcb->mutex));
    
    // Clean up synchronization primitives
    pthread_mutex_destroy(&(bcb->mutex));
    pthread_cond_destroy(&(bcb->can_add_orders));
    pthread_cond_destroy(&(bcb->can_get_orders));
    
    // Free allocated memory
    free(bcb->orders);
    free(bcb);
    
    printf("Restaurant is closed!\n");
}

int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
    pthread_mutex_lock(&(bcb->mutex));
    
    // Wait while the queue is full
    while (IsFull(bcb)) {
        pthread_cond_wait(&(bcb->can_add_orders), &(bcb->mutex));
    }
    
    // Add the order to the queue
    bcb->orders[bcb->tail] = order;
    bcb->tail = (bcb->tail + 1) % bcb->max_size;
    bcb->num_orders_received++;
    
    // Signal that a new order is available
    pthread_cond_signal(&(bcb->can_get_orders));
    
    pthread_mutex_unlock(&(bcb->mutex));
    return bcb->num_orders_received;
}

Order* GetOrder(BENSCHILLIBOWL* bcb) {
    pthread_mutex_lock(&(bcb->mutex));
    
    // If we've handled all expected orders, return NULL
    if (bcb->num_orders_handled >= bcb->expected_num_orders) {
        pthread_mutex_unlock(&(bcb->mutex));
        return NULL;
    }
    
    // Wait while the queue is empty
    while (IsEmpty(bcb) && bcb->num_orders_handled < bcb->expected_num_orders) {
        pthread_cond_wait(&(bcb->can_get_orders), &(bcb->mutex));
    }
    
    // Double check if we've handled all orders after waiting
    if (bcb->num_orders_handled >= bcb->expected_num_orders) {
        pthread_mutex_unlock(&(bcb->mutex));
        return NULL;
    }
    
    // Get the order from the queue
    Order* order = bcb->orders[bcb->head];
    bcb->head = (bcb->head + 1) % bcb->max_size;
    bcb->num_orders_handled++;
    
    // Signal that there's space in the queue
    pthread_cond_signal(&(bcb->can_add_orders));
    
    pthread_mutex_unlock(&(bcb->mutex));
    return order;
}

bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->head == bcb->tail;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
    return ((bcb->tail + 1) % bcb->max_size) == bcb->head;
}

void AddOrderToBack(Order **orders, Order *order) {
    // Note: This function is not needed as we're using a circular buffer implementation
}
