#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

typedef struct nand nand_t;
typedef struct connection_pair con_pair;

// A pair for connection between gates
struct connection_pair
{
    nand_t* gate;
    unsigned k; // Index in input/output array.
    
};

struct nand
{
    unsigned inputs_count;
    unsigned outputs_count;
    unsigned outputs_size; // Size of allocated memory
    con_pair** nand_inputs; // Inputs from other nand gates
    bool const** signal_inputs; // Inputs from boolean signals
    con_pair** outputs; // Outputs to other nand gates
    int visited; // For DFS 0 - unvisited 1 - counting 2 - counted
    ssize_t crit; // Length of critical path
    bool output_signal;
};

// Creating a new nand gate.
nand_t* nand_new(unsigned n)
{
    nand_t* new_nand = malloc(sizeof(nand_t));
    
    // Memory allocate error.
    if (new_nand == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }
    
    // Setting initial values.
    new_nand->inputs_count = n;
    new_nand->outputs_count = 0;
    new_nand->outputs_size = 0;
    new_nand->nand_inputs = malloc(n * sizeof(con_pair*)); // Pamiętaj, że nie alokujesz tu pamięci dla outputs
    new_nand->signal_inputs = malloc(n * sizeof(bool*));
    new_nand->outputs = NULL;
    new_nand->visited = 0;
    new_nand->crit = 0;
    new_nand->output_signal = false;
    
    if (new_nand->nand_inputs == NULL || new_nand->signal_inputs == NULL)
    {
        free(new_nand->nand_inputs);
        free(new_nand->signal_inputs);
        free(new_nand);
        errno = ENOMEM;
        return NULL;
    }
    
    // Setting initial NULL values for both arrays.
    for (unsigned i = 0; i < n; i++)
    {
        new_nand->nand_inputs[i] = NULL;
        new_nand->signal_inputs[i] = NULL;
    }
    
    return new_nand;
}

// Returns index on first empty output array index or
// outputs_size if there are no free places.
unsigned first_empty_output(nand_t* g)
{
    // If array is full.
    if (g->outputs_count == g->outputs_size)
    {
        return g->outputs_size;
    }
    
    // Finding first position with NULL.
    for (unsigned i = 0; i < g->outputs_size; i++)
    {
        if(g->outputs[i] == NULL)
        {
            return i;
        }
    }
    
    return -1;
}

// A function to disconnect a nand input from a gate.
// Note that it also works for NULL values in g->nand_inputs.
void nand_disconnect_input(nand_t* g, unsigned k)
{
    if (g->nand_inputs[k] == NULL)
        return;
    
    // Freeing and setting pointers to NULL's.
    free(g->nand_inputs[k]->gate->outputs[g->nand_inputs[k]->k]);
    g->nand_inputs[k]->gate->outputs[g->nand_inputs[k]->k] = NULL;
    g->nand_inputs[k]->gate->outputs_count--;
    free(g->nand_inputs[k]);
    g->nand_inputs[k] = NULL;
}

// A function to disconnect a nand output from a gate.
// Notice that it also works for NULL values in g->outputs.
void nand_disconnect_output(nand_t* g, unsigned k)
{
    if (g->outputs[k] == NULL)
        return;
        
    // Freeing and setting pointers to NULL's.
    free(g->outputs[k]->gate->nand_inputs[g->outputs[k]->k]);
    g->outputs[k]->gate->nand_inputs[g->outputs[k]->k] = NULL;
    free(g->outputs[k]);
    g->outputs[k] = NULL;
    g->outputs_count--;
}

// Deleting a nand gate. Note that this function
// should not delete pointers inside signal_inputs
// as they were not allocated by this library.
void nand_delete(nand_t* g)
{
    // If wrong data.
    if (g == NULL)
        return;
    
    // Removing all memory related to inputs of g.    
    for (unsigned i = 0; i < g->inputs_count; i++)
    {
        nand_disconnect_input(g, i);
    }
    
    // Removing all memory related to outputs of g.
    for (unsigned i = 0; i < g->outputs_size; i++)
    {
        nand_disconnect_output(g, i);
    }
    
    free(g->nand_inputs);
    free(g->signal_inputs);
    free(g->outputs);
    free(g);
}

// A function to connect gate_out output
// to g_in input at index k.
int nand_connect_nand(nand_t* g_out, nand_t* g_in, unsigned k)
{
    // If wrong data.
    if (g_out == NULL || g_in == NULL || k >= g_in->inputs_count)
    {
        errno = EINVAL;
        return -1;
    }
    
    con_pair* p_out = malloc(sizeof(con_pair));
    
    // Memory allocate error.
    if (p_out == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    
    // Memory allocate error.
    con_pair* p_in = malloc(sizeof(con_pair));
    
    if (p_in == NULL)
    {
        free(p_out);
        errno = ENOMEM;
        return -1;
    }
    
    // First empty index in g_out->outputs or
    // g_out->outputs_size if there is no more space.
    unsigned first_empty = first_empty_output(g_out);
    
    // If there is no more space in g_out->outputs.
    if (first_empty == g_out->outputs_size)
    {
        // In case realloc puts NULL to g_out->outputs.
        con_pair** temp_ptr = &(*g_out->outputs);
        
        g_out->outputs = realloc(g_out->outputs, (g_out->outputs_size + 100) * sizeof(con_pair*));
        
        // Memory allocate error.
        if (g_out->outputs == NULL)
        {
            // Retrieving pointer.
            g_out->outputs = temp_ptr;
            
            free(p_in);
            free(p_out);
            
            errno = ENOMEM;
            return -1;
        }
    }
    
    // Disconnecting previous signal
    nand_disconnect_input(g_in, k);
    g_in->signal_inputs[k] = NULL;
    
    // Initial value for new array field.
    g_out->outputs[g_out->outputs_size] = NULL;
    g_out->outputs_size++;
    
    // Creating new connection(s).
    p_out->gate = g_in;
    p_out->k = k;
    p_in->gate = g_out;
    p_in->k = first_empty;
    g_out->outputs_count++;
    
    // Adding connection(s);
    g_out->outputs[first_empty] = p_out;
    g_in->nand_inputs[k] = p_in;
    
    return 0;
}

// A function to connect a boolean
// signal to gate g on k input.
int nand_connect_signal(bool const* s, nand_t* g, unsigned k)
{
    // If wrong data.
    if (s == NULL || g == NULL || k >= g->inputs_count)
    {
        errno = EINVAL;
        return -1;
    }
    
    // Disconnecting previous signal;
    nand_disconnect_input(g, k);
    // Connecting new signal;
    g->signal_inputs[k] = s;
    
    return 0;
}

// A helper function for max of two
// values with type ssize_t.
ssize_t max(ssize_t a, ssize_t b)
{
    return (a > b) ? a : b;
}

bool dfs(nand_t* g)
{
    // Setting g "vertice" to "counting".
    g->visited = 1;
    
    for (unsigned i = 0; i < g->inputs_count; i++)
    {
        // Check for a cycle.
        if (g->nand_inputs[i] != NULL && g->nand_inputs[i]->gate->visited == 1)
            return false;
        
        // Check for an empty input.
        if (g->nand_inputs[i] == NULL && g->signal_inputs[i] == NULL)
            return false;
        
        if (g->nand_inputs[i] != NULL)
        {
            if (g->nand_inputs[i]->gate->visited == 0)
            {
                // Going deeper into dfs. If it returns false
                // it means a whole dfs is false so this function
                // also returns false.
                if (dfs(g->nand_inputs[i]->gate) == false)
                    return false;
            }
        }
    }
    
    // Initially false but if any input is false
    // then it turns into true.
    bool output_signal = false;
    // A maximum of critical paths on inputs of current gate.
    ssize_t maxim = 0;
    
    // Checking for all signals on input to evaluate
    // a signal of current gate.
    for (unsigned i = 0; i < g->inputs_count; i++)
    {
        if (g->nand_inputs[i] != NULL)
        {
            if (g->nand_inputs[i]->gate->output_signal == false)
                output_signal = true;
        }
        else
        {
            if (*g->signal_inputs[i] == false)
                output_signal = true;
        }
    }
    
    // Setting final signal to gate.
    g->output_signal = output_signal;
    
    // Evaluating critical path length. Notice
    // that if an input is a boolean input, we can
    // omit that case and still get a proper evaluation.
    for (unsigned i = 0; i < g->inputs_count; i++)
    {
        if (g->nand_inputs[i] != NULL)
            maxim = max(maxim, g->nand_inputs[i]->gate->crit);
    }
    
    // Setting final critical path length to gate.
    g->crit = g->inputs_count == 0 ? 0 : 1 + maxim;
    
    // Setting g "vertice" to "counted".
    g->visited = 2;
    
    // DFS for g succeeded.
    return true;
}

// A function to clear "visited" after DFS.
void dfs_clear(nand_t* g)
{
    g->visited = 0;
    
    for (unsigned i = 0; i < g->inputs_count; i++)
    {
        if (g->nand_inputs[i] != NULL && g->nand_inputs[i]->gate->visited != 0)
        {
            dfs_clear(g->nand_inputs[i]->gate);
        }
    }
}

// A function to evaluate signals for gates system
// and critical path length.
ssize_t nand_evaluate(nand_t** g, bool* s, size_t m)
{
    // If wrong data.
    if (g == NULL || s == NULL || m == 0)
    {
        errno = EINVAL;
        return -1;
    }
    
    // If wrong data.
    for (size_t i = 0; i < m; i++)
    {
        if (g[i] == NULL)
        {
            errno = EINVAL;
            return -1;
        }
    }
    
    // DFS starting at all gates in g.
    for (size_t i = 0; i < m; i++)
    {
        bool dfs_success = true;
        
        if (g[i]->visited == 0)
            dfs_success = dfs(g[i]);
        
        // DFS fail (cycle, empty input)    
        if (dfs_success == false)
        {
            // Clearing "visited" after DFS.
            for (size_t i=0; i<m; i++)
            {
                if (g[i]->visited != 0)
                {
                    dfs_clear(g[i]);
                }
            }
	                
            errno = ECANCELED;
            return -1;
        }
    }
    
    // A maximum of critical paths for all
    // gates in g.
    ssize_t maxim = 0;
    
    for (size_t i = 0; i < m; i++)
    {
        s[i] = g[i]->output_signal;
        maxim = max(maxim, g[i]->crit);
    }
    
    // Clearing "visited" after DFS.
    for (size_t i = 0; i < m; i++)
    {
        if (g[i]->visited != 0)
        {
            dfs_clear(g[i]);
        }
    }
    
    // Length of critical path for gates system g.
    return maxim;
}

// A function to get number of how many gates
// are connected to the output of g.
ssize_t nand_fan_out(nand_t const *g)
{
    // If wrong data.
    if (g == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    
    return g->outputs_count;
}

// A function to get a pointer of signal connected
// to g on input k.
void* nand_input(nand_t const *g, unsigned k)
{
    // If wrong data.
    if (g == NULL || k >= g->inputs_count)
    {
        errno = EINVAL;
        return NULL;
    }
    
    // Returning a proper signal.
    if (g->signal_inputs[k] != NULL)
    {
        return (void*)g->signal_inputs[k];
    }
    else if (g->nand_inputs[k] != NULL)
    {
        return g->nand_inputs[k]->gate;
    }
    else
    {
        // It means gate has an empty input.
        errno = 0;
        return NULL;
    }
}

// A function to iterate over gates connected
// to the output of gate g.
nand_t* nand_output(nand_t const *g, ssize_t k)
{
    // If wrong data. Actually it is indefinite
    // what this function should return in such
    //state, therefore it returns NULL.
    if (g == NULL || k < 0 || k >= nand_fan_out(g))
    {
        return NULL;
    }
    
    // A counter because there may be some
    // empty (NULL) positions in g->outputs.
    ssize_t k_counter = -1;
    
    for (unsigned i = 0; i < g->outputs_size; i++)
    {
        if (g->outputs[i] != NULL)
            k_counter++;
        
        // Found k output.    
        if (k_counter == k)
        {
            return g->outputs[i]->gate;
        }
    }
    
    return NULL;
}