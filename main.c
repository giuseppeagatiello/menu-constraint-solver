#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DISH_FALSE  0
#define DISH_TRUE   1
#define DISH_IDK    2

#define EMP_CONTRADICTION 0
#define EMP_SATISFIED     1
#define EMP_FORCED        2
#define EMP_OPEN          3

/* Data structures */
// Hash table
typedef struct {
    const char *name;
    unsigned int dish_id;
} HashNode;

typedef struct {
    HashNode *data;
    size_t capacity;
} DishHash;

// A dish and its current assignment
typedef struct {
    unsigned int id;
    char *name;
    int selected;
} Dish;

// Dynamic array of dishes
typedef struct{
    Dish *data;
    size_t size;        // Number of stored dishes
    size_t capacity;    // Allocated slots
} Dishvector;

// Preferences
typedef struct {            // {dish_id, wanted}
    unsigned int dish_id;
    int wanted;
} Preference;

typedef struct{
    Preference *data;
    size_t size;
    size_t capacity;
} PreferenceVector;

typedef struct {
    unsigned int *employee_ids;
    size_t size;
    size_t capacity;
} OccurrenceVector;

// Each employee's preferences occupy a contiguous slice of the preference array.
typedef struct{
    unsigned int id;
    size_t pref_start;
    size_t pref_count;
} Employee;

typedef struct {
    Employee *data;
    size_t size;
    size_t capacity;
} EmployeeVector;

/* Function declarations */

/* Dish vector */
void dish_vector_init(Dishvector *vector);
int dish_vector_push(Dishvector *vector, const char*name);
int dish_vector_find(Dishvector *vector, const char*name, unsigned int *id);
void dish_vector_destroy(Dishvector *vector);

/* Individual dishes */
int dish_init(Dish *dish, unsigned int id, const char *name);
void dish_destroy(Dish *dish);
void dish_select_idk(Dish *dish);
void dish_select(Dish *dish);
void dish_unselect(Dish *dish);

/* Employee stack */
void employee_vect_init(EmployeeVector *vect);
int employee_vect_push(EmployeeVector *vect, unsigned int id, size_t pref_start);
int employee_vect_pop(EmployeeVector *vect);
void employee_vect_destroy(EmployeeVector *vect);

/* Preferences */
void preference_vect_init(PreferenceVector *vect);
int preference_vect_push(PreferenceVector *vect, unsigned int dish_id,int wanted);
void preference_vect_destroy(PreferenceVector *vect);

int occurrence_vect_push(OccurrenceVector *vect,unsigned int employee_id); // TODO: review this function.
void occurrence_vect_destroy(OccurrenceVector *vect, size_t dish_count);

/* Input */
char *read_line(FILE *stream);

/* Debug output */
void print_loaded_data(const Dishvector *dishes, const EmployeeVector *employees, const PreferenceVector *preferences);

/* Hash table */
void dish_hash_init(DishHash *table, const Dishvector *dishes);
int dish_hash_insert(DishHash *table,const char *name,unsigned int dish_id);
int dish_hash_find(const DishHash *table,const char *name,unsigned int *dish_id);
void dish_hash_destroy(DishHash *table);
size_t hash_function(const char *name);

/* Solver */
int evaluate_preference(const Dishvector *dishes,const Preference *preference);
int evaluate_employee(const Dishvector *dishes,const PreferenceVector *preferences, const Employee *employee, size_t *forced_pref, size_t *idk_count_out); /* OR */
int evaluate_constraints(Dishvector *dishes,const PreferenceVector *preferences, const EmployeeVector *employees,const OccurrenceVector *dish_occurrences, unsigned int trail[], size_t *trail_size, Preference *branch_preference);
int solver(Dishvector *dishes,const PreferenceVector *preferences, const EmployeeVector *employees,const OccurrenceVector *dish_occurrences, unsigned int trail[], size_t trail_size);
/* Entry point */

int main(){
    size_t i;
    unsigned int employee_id;
    unsigned int id;
    char *input_line;
    char *token;
    int result;
    size_t trail_size;
    Dishvector dish_vector;
    EmployeeVector employee_vect;
    PreferenceVector preference_vect;
    DishHash dish_hash;

    dish_vector_init(&dish_vector);
    employee_vect_init(&employee_vect);
    preference_vect_init(&preference_vect);

    employee_id = 0;

    /* The first line lists the available dishes. */
    input_line = read_line(stdin);
    token = strtok(input_line, " ");
    while(token != NULL){              
        dish_vector_push(&dish_vector, token);
        token = strtok(NULL, " ");
    }
    free(input_line);

    dish_hash_init(&dish_hash, &dish_vector);
    OccurrenceVector *dish_occurrences = calloc(dish_vector.size, sizeof(*dish_occurrences));

    /* Each remaining line contains one employee's preferences. */
    while((input_line = read_line(stdin)) != NULL){
        employee_vect_push(&employee_vect, employee_id, preference_vect.size); /* Start at the next free preference slot. */
        token = strtok(input_line, " ");
        while(token != NULL){
            char *dish_name;
            int wanted;
            if (token[0] == '-') {
                wanted = 0;
                dish_name = token + 1;
            } else {
                wanted = 1;
                dish_name = token;
            }
            if(dish_hash_find(&dish_hash, dish_name, &id)){
                preference_vect_push(&preference_vect, id, wanted);
                occurrence_vect_push(&dish_occurrences[id],employee_id);
                employee_vect.data[employee_vect.size - 1].pref_count++;
            }
            token = strtok(NULL, " ");
        }
        free(input_line);
        employee_id++;
    }
    /* Solve the constraints and keep a trail of assignments for backtracking. */
    unsigned int trail[dish_vector.size];
    trail_size = 0;
    i = 0;

    result = solver(&dish_vector, &preference_vect, &employee_vect, dish_occurrences, trail, trail_size);
    if(result == EMP_SATISFIED){
        printf("OK\n");
    } else{
        printf("KO\n");
        while(result != EMP_SATISFIED && i < employee_vect.size){
            i++;
            preference_vect.size = preference_vect.size - employee_vect.data[employee_vect.size - 1].pref_count;
            printf("-%zu\n", i);
            if(employee_vect_pop(&employee_vect)){      /* Retry after removing the last employee. */
                result =  solver(&dish_vector, &preference_vect, &employee_vect, dish_occurrences, trail, trail_size);
            } else{                                     /* Stop if there was no employee to remove. */
                result = 0;
                break;
            }
        }
        if(result == EMP_SATISFIED){
            printf("OK\n");
        }
    }

    /* Release allocated memory. */
    occurrence_vect_destroy(dish_occurrences, dish_vector.size);
    employee_vect_destroy(&employee_vect);
    dish_vector_destroy(&dish_vector);
    dish_hash_destroy(&dish_hash);
    preference_vect_destroy(&preference_vect);
    return 0;
}

/* Hash table */
void dish_hash_init(DishHash *table, const Dishvector *dishes){
    size_t i;

    table->capacity = dishes->size * 2;

    table->data = calloc(table->capacity, sizeof(*table->data)); /* Initialize empty slots with NULL names. */

    for(i = 0; i < dishes->size; i++){
        dish_hash_insert(table, dishes->data[i].name, dishes->data[i].id);
    }
}

int dish_hash_insert(DishHash *table,const char *name,unsigned int dish_id){
    size_t index;

    index = hash_function(name) % table->capacity;
    while(table->data[index].name != NULL){
        index = (index + 1) % table->capacity;
    }
    table->data[index].dish_id = dish_id;
    table->data[index].name = name;

    return 1;
}

size_t hash_function(const char *name){
    size_t hash = 14695981039346656037ULL;

    while(*name != '\0'){
        hash ^= (unsigned char)*name;
        hash *= 1099511628211ULL;
        name++;
    }

    return hash;
}

int dish_hash_find(const DishHash *table,const char *name,unsigned int *dish_id){
    size_t index;
    index = hash_function(name) % table->capacity;
    while(table->data[index].name != NULL){
        if(strcmp(table->data[index].name, name) == 0){
            *dish_id = table->data[index].dish_id;
            return 1;
        }
        index =(index + 1) % table->capacity;
    }
    return 0;
}

void dish_hash_destroy(DishHash *table){
    free(table->data);
    table->data = NULL;
    table->capacity = 0;
}

/* Dish vector */
void dish_vector_init(Dishvector *vector){
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

int dish_vector_push(Dishvector *vector, const char*name){
    if(vector->size == vector->capacity){
        Dish *temporary;
        size_t new_capacity;

        if (vector->capacity == 0) {
            new_capacity = 4;
        } else {
            new_capacity = 2 * vector->capacity;
        }

        
        temporary = realloc(vector->data, new_capacity * sizeof(*vector->data));
        
        // Keep the existing array if reallocation fails.
        if(temporary == NULL){
            return 0;
        }

        vector->data = temporary;
        vector->capacity = new_capacity;
    }

    if(dish_init(&vector->data[vector->size],(unsigned int) vector->size, name) == 0){
            return 0;
        }
    
    vector->size++;

    return 1;
}

void dish_vector_destroy(Dishvector *vector){
    size_t i;

    for(i = 0; i < vector->size; i++){
        dish_destroy(&vector->data[i]);
    }

    free(vector->data);

    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

/* Individual dishes */

int dish_init(Dish *dish, unsigned int id, const char *name){
    size_t len;
    len = strlen(name) + 1;
    dish->id = id;
    
    dish->name = malloc(len);
    if(dish->name==NULL){
        return 0;
    }
    
    strcpy(dish->name, name);

    dish->selected = 2;

    return 1;
}

void dish_destroy(Dish *dish){
    free(dish->name);
    dish->name=NULL;
}

void dish_select_idk(Dish *dish){
    dish->selected = 2;
}
void dish_select(Dish *dish){
        dish->selected = 1;
}
void dish_unselect(Dish *dish){
        dish->selected = 0;
}



/* Employee stack */
void employee_vect_init(EmployeeVector *vect){
    vect-> data = NULL;
    vect-> size = 0;
    vect ->capacity = 0;
}

/* Push an employee. */
int employee_vect_push(EmployeeVector *vect, unsigned int id, size_t pref_start){
    if(vect->size == vect->capacity){
        Employee *temp;
        size_t new_capacity;

        if(vect->capacity == 0){
            new_capacity = 4;
        } else{
            new_capacity = 2 * vect->capacity;
        }

        temp = realloc(vect->data, new_capacity * sizeof(*vect->data));
        if(temp == NULL){
            return 0;
        }

        vect->data = temp;
        vect->capacity = new_capacity;
    }
    vect->data[vect->size].id = id;
    vect->data[vect->size].pref_start = pref_start;
    vect->data[vect->size].pref_count = 0;
    vect->size++;
    return 1;
}

/* Pop the last employee. */
int employee_vect_pop(EmployeeVector *vect){
    if(vect->size == 0){
        return 0;
    } 

    vect->size--;
    return 1;
}

/* Release the employee array. */
void employee_vect_destroy(EmployeeVector *vect){
    free(vect->data);

    vect->data = NULL;
    vect->size = 0;
    vect->capacity = 0;
}

/* Read a line of arbitrary length. */
char *read_line(FILE *stream){
    char *line;
    int c;
    size_t length;
    size_t capacity;

    /* Start with a small buffer and grow it as needed. */
    length = 0;
    capacity = 4;

    /* Allocate the initial buffer. */
    line = malloc(capacity);
    if(line == NULL){
        return NULL;
    }

    c = fgetc(stream);

    while (c != '\n' && c != EOF){

        /* Keep room for the terminating null character. */
        if(length+1 >= capacity){
            char * temporary;
            size_t new_capacity;

            new_capacity = 2 * capacity;
            temporary = realloc(line, new_capacity);

            // Release the buffer if reallocation fails.
            if(temporary == NULL){
                free(line);
                return NULL;
            }

            line = temporary;
            capacity = new_capacity;
        }
        line[length] = c;
        length++;
        c = fgetc(stream);
    }
    /* EOF before any characters were read. */
    if(c == EOF && length == 0){
        free(line);
        return NULL;
    }
    /* Terminate the string. */
    line[length] = '\0';

    return line;
}

/* Preference vector */
void preference_vect_init(PreferenceVector *vect){
    vect->data = NULL;
    vect->size = 0;
    vect->capacity = 0;
}

int preference_vect_push(PreferenceVector *vect, unsigned int dish_id,int wanted){
    if(vect->size == vect->capacity){
        Preference *temp;
        size_t new_capacity;

        if(vect->capacity == 0){
            new_capacity = 4;
        } else{
            new_capacity = 2 * vect->capacity;
        }

        temp = realloc(vect->data, new_capacity * sizeof(*vect->data));
        if(temp == NULL){
            return 0;
        }

        vect->data = temp;
        vect->capacity = new_capacity;
    }
    vect->data[vect->size].dish_id = dish_id;
    vect->data[vect->size].wanted = wanted;
    vect->size++;
    return 1;
}

void preference_vect_destroy(PreferenceVector *vect){
    free(vect->data);

    vect->data = NULL;
    vect->size = 0;
    vect->capacity = 0;
}

int occurrence_vect_push(OccurrenceVector *vect,unsigned int employee_id){
    if (vect->size == vect->capacity) {
        size_t new_capacity;
        unsigned int *temporary;
        if (vect->capacity == 0) {
            new_capacity = 4;
        } else {
            new_capacity = vect->capacity * 2;
        }
        temporary = realloc(
            vect->employee_ids,
            new_capacity * sizeof(*temporary)
        );
        if (temporary == NULL) {
            return 0;
        }
        vect->employee_ids = temporary;
        vect->capacity = new_capacity;
    }

    vect->employee_ids[vect->size] = employee_id;
    vect->size++;

    return 1;
}

void occurrence_vect_destroy(OccurrenceVector *vect, size_t dish_count){
    for(size_t i = 0; i < dish_count; i++){
        free(vect[i].employee_ids);
    }
    free(vect);
}

/* Constraint evaluation and backtracking */
int evaluate_preference(const Dishvector *dishes,const Preference *preference){   /* Evaluate a preference under the current assignment. */
    const Dish *dish = &dishes->data[preference->dish_id];
    if(dish->selected == DISH_IDK){
        return DISH_IDK;
    }
    if(dish->selected == preference->wanted){
        return DISH_TRUE;
    }
    return DISH_FALSE;
}

int evaluate_employee(const Dishvector *dishes,const PreferenceVector *preferences, const Employee *employee, size_t *forced_pref, size_t *idk_count_out){
    size_t i, idk_count, last_idk;
    int evaluation;
    idk_count = 0;

    for(i=0; i < employee->pref_count; i++){
        evaluation = evaluate_preference(dishes, &preferences->data[employee->pref_start + i]);
        if(evaluation == 1){
            return EMP_SATISFIED;
        }
        if(evaluation == 2){
            idk_count++;
            last_idk = employee->pref_start + i;
        }
    }
    if(idk_count == 1){
        *forced_pref = last_idk;
        return EMP_FORCED;
    }
    if(idk_count > 1){
        *forced_pref = last_idk;
        *idk_count_out = idk_count; /* Number of undecided preferences for this employee. */
        return EMP_OPEN;
    }
    return EMP_CONTRADICTION;
}

int evaluate_constraints(Dishvector *dishes,const PreferenceVector *preferences, const EmployeeVector *employees,const OccurrenceVector *dish_occurrences, unsigned int trail[], size_t *trail_size, Preference *branch_preference){
    size_t i, forced_pref;
    bool all_sat, branch_found;
    int state;
    size_t idk_count, best_idk_count;
    size_t employee_count = employees->size;

    if (employee_count == 0) {
        return EMP_SATISFIED;
    }
    unsigned int *queue = malloc(employee_count * sizeof(*queue));
    bool *in_queue = calloc(employee_count, sizeof(*in_queue));
    int *employee_state = malloc(employee_count * sizeof(*employee_state));
    size_t *open_pref = malloc(employee_count * sizeof(*open_pref));
    size_t *open_idk_count = malloc(employee_count * sizeof(*open_idk_count));

    size_t head = 0;        // Next item to dequeue
    size_t tail = 0;        // Next insertion slot
    size_t pending = 0;     // Number of employees in the queue

    for (i = 0; i < employee_count; i++) {
        queue[tail] = (unsigned int)i;
        tail = (tail + 1) % employee_count; // Wrap around at the end of the buffer.
        pending++;
        in_queue[i] = true;
    }

    while(pending > 0){
        unsigned int employee_id = queue[head];     // Dequeue the next employee.
        head = (head + 1) % employee_count;
        pending--;

        in_queue[employee_id] = false;
        state = evaluate_employee(dishes, preferences, &employees->data[employee_id], &forced_pref, &idk_count);
        employee_state[employee_id] = state;
        // Handle the current constraint state.
        if(state == EMP_CONTRADICTION){
            free(queue);
            free(in_queue);
            free(employee_state);
            free(open_pref);
            free(open_idk_count);
            return EMP_CONTRADICTION;
        }
        if(state == EMP_OPEN){
            open_pref[employee_id]= forced_pref;
            open_idk_count[employee_id] = idk_count;
        }
        if(state == EMP_FORCED){
            unsigned int changed_dish = preferences->data[forced_pref].dish_id;
            dishes->data[changed_dish].selected = preferences->data[forced_pref].wanted;

            trail[*trail_size] = changed_dish;
            (*trail_size)++;

            const OccurrenceVector *affected = &dish_occurrences[changed_dish];
            for (size_t j = 0; j < affected->size; j++){
                unsigned int affected_employee = affected->employee_ids[j];

                /* Ignore employees removed by main after a KO result. */
                if (affected_employee >= employee_count) {
                    continue;
                }

                /* Avoid adding the same employee to the queue twice. */
                if (!in_queue[affected_employee]) {
                    queue[tail] = affected_employee;
                    tail = (tail + 1) % employee_count;
                    pending++;
                    in_queue[affected_employee] = true;
                }
            }
        }
    }

    all_sat = true;
    branch_found = false;
    for(i = 0; i < employee_count; i++){
        if(employee_state[i] != EMP_SATISFIED){
            all_sat = false;
        }
        if(employee_state[i] == EMP_OPEN){
            if(!branch_found || open_idk_count[i] < best_idk_count){
                best_idk_count = open_idk_count[i];
                *branch_preference = preferences->data[open_pref[i]];
                branch_found = true;
            }
        }
    }
    free(queue);
    free(in_queue);
    free(employee_state);
    free(open_pref);
    free(open_idk_count);

    if(all_sat){
        return EMP_SATISFIED;
    }
    return EMP_OPEN;
}

int solver(Dishvector *dishes,const PreferenceVector *preferences, const EmployeeVector *employees,const OccurrenceVector *dish_occurrences, unsigned int trail[], size_t trail_size){
    int evaluation;
    Preference branch_preference;
    size_t entry_mark, branch_mark;

    entry_mark = trail_size;
    evaluation = evaluate_constraints(dishes, preferences, employees, dish_occurrences, trail, &trail_size, &branch_preference);
    if(evaluation == EMP_CONTRADICTION){
        while(trail_size > entry_mark){
            trail_size--;
            dish_select_idk(&dishes->data[trail[trail_size]]);
        }
        return EMP_CONTRADICTION;
    }
    if(evaluation == EMP_SATISFIED){
        return EMP_SATISFIED;
    }
    if(evaluation == EMP_OPEN){
        /* Branch on the selected undecided preference. */
        branch_mark = trail_size;

        if(branch_preference.wanted == DISH_FALSE){
            /* Try excluding the dish. */
            dish_unselect(&dishes->data[branch_preference.dish_id]);
        } else{
            /* Try including the dish. */
            dish_select(&dishes->data[branch_preference.dish_id]);
        }
        trail[trail_size] = branch_preference.dish_id;
        trail_size++;
        if(solver(dishes, preferences, employees, dish_occurrences, trail, trail_size) == EMP_SATISFIED){
            return EMP_SATISFIED;
        }
        while(trail_size > branch_mark){
            trail_size--;
            dish_select_idk(&dishes->data[trail[trail_size]]);
        }

        if(branch_preference.wanted == DISH_FALSE){
            /* Try including the dish. */
            dish_select(&dishes->data[branch_preference.dish_id]);
        } else{
        /* Try excluding the dish. */
            dish_unselect(&dishes->data[branch_preference.dish_id]);
        }
        trail[trail_size] = branch_preference.dish_id;
        trail_size++;
        if(solver(dishes, preferences, employees, dish_occurrences, trail, trail_size) == EMP_SATISFIED){
            return EMP_SATISFIED;
        }
        while(trail_size > branch_mark){
            trail_size--;
            dish_select_idk(&dishes->data[trail[trail_size]]);
        }
    }

    /* Undo every assignment made by this solver call. */
    while(trail_size > entry_mark){
        trail_size--;
        dish_select_idk(&dishes->data[trail[trail_size]]);
    }
    return EMP_CONTRADICTION;
}
