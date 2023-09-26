#include "simulator.h"

int block_size;
int l1_size;
int l1_assoc;
int l1_num_sets;
int l2_size;
int l2_assoc;
int l2_num_sets;
Replacement replacement_policy;
Inclusion inclusion_property;
char *trace_file;
uint mask;

int countRead = 0;
int countWrite = 0;

int countReadL2 = 0;
int countWriteL2 = 0;

int readHit = 0;
int readMiss = 0;
int writeHit = 0;
int writeMiss = 0;

int readHitL2 = 0;
int readMissL2 = 0;
int writeHitL2 = 0;
int writeMissL2 = 0;

int totalCount = 0;

int writeback = 0;
int writebackL2 = 0;
int invalid_wb = 0;

int index_size_l2 = 0;
int index_size_l1 = 0;

// Prefetched start
typedef struct{
    unsigned int tag;
    int valid;
    uint address;
}PrefetchBlock;

int L1prefetch_hits = 0;
int L1prefetch_misses = 0;
int L2prefetch_hits = 0;
int L2prefetch_misses = 0;

// Prefetch End

// L1/L2 matrices structure
Block **matrix = NULL;
Block **matrixL2 = NULL;

Vector memory_addresses;
Vector optimal_set;

int fifoCount = 0;

int main(int argc, char *argv[]) {

    // Read for command line arguments
    FILE *trace_file_open;
    if (argc != 9) {
        usage();
    }

    block_size = atoi(argv[1]);
    l1_size = atoi(argv[2]);
    l1_assoc = atoi(argv[3]);
    l2_size = atoi(argv[4]);
    l2_assoc = atoi(argv[5]);
    replacement_policy = atoi(argv[6]);
    inclusion_property = atoi(argv[7]);
    trace_file = argv[8];
    trace_file_open = fopen(argv[8], "r");
    printInput();

    mask = gen_mask(block_size);

    // Initialize matrices and parse the trace file
    init();
    printFile(trace_file_open);

    // Close everything
    free_everything();
    fclose(trace_file_open);
}

void free_everything() {
    // Free matrices
    for (int i = 0; i < l1_num_sets; i++)
        free(matrix[i]);
    free(matrix);

    if (matrixL2 != NULL) {
        for (int i = 0; i < l2_num_sets; i++) 
            free(matrixL2[i]);
        free(matrixL2);
    }

    free(optimal_set.list->ar);
    free(optimal_set.list);

    free(memory_addresses.list->ar);
    free(memory_addresses.list);
}

void init() {
    // Initialize matrices
    l1_num_sets = l1_size / (l1_assoc  * block_size);

    index_size_l1 = log2(l1_num_sets);

    matrix = malloc(sizeof(Block *) * l1_num_sets);
    
    for (int i = 0; i < l1_num_sets; i++) {
        matrix[i] = calloc(l1_assoc, sizeof(Block));
    }

    if (l2_size) {
        l2_num_sets = l2_size / (l2_assoc * block_size);

        index_size_l2 = log2(l2_num_sets);

        matrixL2 = malloc(sizeof(Block *) * l2_num_sets);
        for (int i = 0; i < l2_num_sets; i++) {
            matrixL2[i] = calloc(l2_assoc, sizeof(Block));
        }
    }

    init_vectors();
}

void init_vectors() {
    // Initialize vectors
    memory_addresses.list = malloc(sizeof(ArrayList));
    memory_addresses.list->cap = DEFAULT_CAP;
    memory_addresses.list->size = 0;
    memory_addresses.list->ar = malloc(sizeof(uint) * DEFAULT_CAP);

    optimal_set.list = malloc(sizeof(ArrayList));
    optimal_set.list->cap = l1_assoc;
    optimal_set.list->size = 0;
    optimal_set.list->ar = malloc(sizeof(uint) * l1_assoc);

    memory_addresses.push = optimal_set.push = append;
    memory_addresses.delete = optimal_set.delete = delete;
    memory_addresses.trim = optimal_set.trim = trim;
    memory_addresses.resize = optimal_set.resize = resize;
    optimal_set.clear = clear;
}

void usage() {
    // Print usage statement
    char *usage_statement = "Usage: ./sim_cache <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file> \n" \
                            "   <BLOCKSIZE> - Block size in bytes. Same block size for all caches in the memory hierarchy. (positive integer)\n" \
                            "   <L1_SIZE> - L1 cache size in bytes. (positive integer) \n" \
                            "   <L1_ASSOC> - L1 set-associativity. 1 is direct-mapped. (positive integer)\n" \
                            "   <L2_SIZE> - L2 cache size in bytes.  0 signifies that there is no L2 cache. (positive integer)\n" \
                            "   <L2_ASSOC> - L2 set-associativity. 1 is direct-mapped. (positive integer)\n" \
                            "   <REPLACEMENT_POLICY> -  0 for LRU, 1 for FIFO, 2 foroptimal.(positive integer)\n" \
                            "   <INCLUSION_PROPERTY> - 0 for non-inclusive, 1 for inclusive. (positive integer)\n" \
                            "   <trace_file> - is the filename of the input trace (string)";
    printf("%s\n", usage_statement);
}

static inline char *convertReplacement(Replacement replacement_policy){
    // Convert replacement policy from int to string
    if (replacement_policy > 2){
        char * emptyChar = "Not Valid";
        return emptyChar;
    }
    char * const replacementStrings[] = {"LRU", "FIFO", "optimal"};
    return replacementStrings[replacement_policy];
}

static inline char *convertInclusion(Inclusion inclusion_property){
    // Convert inclusion property from int to string
    if (inclusion_property > 1){
        char * emptyChar = "Not Valid";
        return emptyChar;
    }
    char * const inclusionStrings[] = {"non-inclusive", "inclusive"};
    return inclusionStrings[inclusion_property];
}

void printInput() {
    // Print input
    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:          %d\n", block_size);
    printf("L1_SIZE:            %d\n", l1_size);
    printf("L1_ASSOC:           %d\n", l1_assoc);
    printf("L2_SIZE:            %d\n", l2_size);
    printf("L2_ASSOC:           %d\n", l2_assoc);
    printf("REPLACEMENT POLICY: %s\n", convertReplacement(replacement_policy));
    printf("INCLUSION PROPERTY: %s\n", convertInclusion(inclusion_property));
    printf("trace_file:         %s\n", basename(trace_file));
}

void printResults() {
    // Print L1 contents and if dirty
    printf("===== L1 contents =====\n");
    for (int x = 0; x < l1_num_sets; x++) {
        printf("Set    %d:", x);
        for (int y = 0; y < l1_assoc; y++) {
            printf("%x %c  ",matrix[x][y].tag,matrix[x][y].dirty);
        }
        printf("\n");
    }
    // Print L2 contents and if dirty
    if (matrixL2 != NULL) {
        printf("===== L2 contents =====\n");
        for (int x = 0; x < l2_num_sets; x++) {
            printf("Set    %d:", x);
            for (int y = 0; y < l2_assoc; y++) {
                printf("%x %c  ", matrixL2[x][y].tag, matrixL2[x][y].dirty);
            }
            printf("\n");
        }
    }
    // Print raw stats
    printf("===== Simulation results (raw) =====\n");
    printf("a. number of L1 reads:        %d\n", countRead);
    printf("b. number of L1 read misses:  %d\n", readMiss);
    printf("c. number of L1 writes:       %d\n", countWrite);
    printf("d. number of L1 write misses: %d\n", writeMiss);
    printf("e. L1 miss rate:              %f\n", (float)(readMiss + writeMiss) / (countRead + countWrite));
    printf("f. number of L1 writebacks:   %d\n", writeback);
    printf("g. number of L2 reads:        %d\n",countReadL2);
    printf("h. number of L2 read misses:  %d\n", readMissL2);
    printf("i. number of L2 writes:       %d\n",countWriteL2);
    printf("j. number of L2 write misses: %d\n", writeMissL2);
#if OPT
    printf("Optimization. number of PrefetchL1 hits: %d\n", L1prefetch_hits);
    printf("Optimization. number of PrefetchL1 misses: %d\n",L1prefetch_misses);
    printf("Optimization. number of PrefetchL2 hits: %d\n", L2prefetch_hits);
    printf("Optimization. number of PrefetchL2 misses: %d\n",L2prefetch_misses);
#endif

    if (matrixL2 == NULL)
        printf("k. L2 miss rate:              %d\n", 0);
    else
        printf("k. L2 miss rate:              %f\n", (float)(readMissL2) / countReadL2);
    
    printf("l. number of L2 writebacks:   %d\n", writebackL2);

    if (matrixL2 == NULL)
        printf("m. total memory traffic:      %d\n", readMiss + writeMiss + writeback);
    else  {
        int mem_count = readMissL2 + writeMissL2 + writebackL2;
        switch (inclusion_property) {
            case inclusive:
                printf("m. total memory traffic:      %d\n", mem_count + invalid_wb);
                break;
            case noninclusive:
                printf("m. total memory traffic:      %d\n", mem_count);
                break;
            default:
                break;
        }
    }
}

uint gen_mask(uint block) {
    // generate mask based on block
    int offset = log2(block);

    return ((0xFFFFFFFF >> offset) << offset);
}

void invalidation(uint addr) {
    // Check to invalidate L1 from eject L2 blocks
    Address tmp = calc_addressing(addr, 1);

    for (int i = 0; i < l1_assoc; i++) {
        if (matrix[tmp.index][i].addr == tmp.addr) {
            if (matrix[tmp.index][i].valid) {
                matrix[tmp.index][i].valid = 0;
                if (matrix[tmp.index][i].dirty == 'D') {
                    invalid_wb++;
                }
            }
            matrix[tmp.index][i].valid = 0;
        }
    }
}

uint optimal_victim(int lvl) {
    // Find optimal victim
    Address tmp;

    for (int i = totalCount; i < memory_addresses.list->size; i++) {
        if (optimal_set.list->size == 1) {
            return optimal_set.list->ar[0];
        }
        tmp = calc_addressing(memory_addresses.list->ar[i], lvl);
        for (int j = 0; j < optimal_set.list->size; j++) {
            if (optimal_set.list->ar[j] == tmp.addr) {
                optimal_set.delete(optimal_set.list, j);
                break;
            }
        }
    }
    return optimal_set.list->ar[0];
}

void optimalFunctionL2(uint addr, uint tag, int index) {
    // Optimal function for L2
    uint victim;
    for (int i = 0; i < l2_assoc; i++) {
        if (matrixL2[index][i].valid) {
            optimal_set.push(optimal_set.list, matrixL2[index][i].addr);
        }
    }

    victim = optimal_victim(2);
    for (int i = 0; i < l2_assoc; i++) {
        if (matrixL2[index][i].addr == victim) {
            if (matrixL2[index][i].dirty == 'D')
                writebackL2++;
            if (inclusion_property == 1)
                invalidation(matrixL2[index][i].addr);
            matrixL2[index][i].tag = tag;
            matrixL2[index][i].addr = addr;
            matrixL2[index][i].replacementCount = 0;
            matrixL2[index][i].valid = 1;
            optimal_set.clear(optimal_set.list);
            break;
        }
    }
}

void lruFunctionL2(uint addr, uint tag, int index){
    // LRU function for L2. Find the largest number and replace with new block.
    int biggest = -1;
    int biggestIndex = 0;
    Address tmp;

    for(int x = 0; x < l2_assoc; x++){
        if(matrixL2[index][x].replacementCount > biggest){
            biggest = matrixL2[index][x].replacementCount;
            biggestIndex = x;
        }
    }
    if(matrixL2[index][biggestIndex].dirty == 'D')
        writebackL2++;
    if (inclusion_property == 1)
        invalidation(matrixL2[index][biggestIndex].addr);

    matrixL2[index][biggestIndex].tag = tag;
    matrixL2[index][biggestIndex].addr = addr;
    matrixL2[index][biggestIndex].replacementCount = 0;
    for(int x = 0; x < l2_assoc; x++){
        if(matrixL2[index][x].tag != tag){
            matrixL2[index][x].replacementCount += 1;
        }
    }
}

void fifoFunctionL2(uint tag, int index, uint addr){
    // FIFO function for L2. Find the smallest number and replace with new block.
    int smallest = 9999999;
    int smallestIndex = 0;
    for(int x = 0; x < l2_assoc; x++){
        if(matrixL2[index][x].replacementCount < smallest){
            smallest = matrixL2[index][x].replacementCount;
            smallestIndex = x;
        }
    }
    if(matrixL2[index][smallestIndex].dirty == 'D')
        writeback++;
    if (inclusion_property == 1)
        invalidation(matrixL2[index][smallestIndex].addr);

    matrixL2[index][smallestIndex].tag = tag;
    matrixL2[index][smallestIndex].addr = addr;
    matrixL2[index][smallestIndex].replacementCount = fifoCount++;
}

void l2Cache(char operation, uint addr){

    if (matrixL2 == NULL)
        return; 

    // Calculate index, tag and offset for L2
    Address tmp = calc_addressing(addr, 2);
    int index = tmp.index, tag = tmp.tag;
#if OPT
    int prefetch_index = (index + 1) % block_size; // Calculate the index of the block to prefetch
    Address prefetch_address = calc_addressing((prefetch_index << block_size), 2); // Calculate the address of the block to prefetch

    int prefetch_flag = 0;
    for (int x = 0; x < l2_assoc; x++) {
        if (matrixL2[prefetch_index][x].valid && matrixL2[prefetch_index][x].tag == prefetch_address.tag) {
            prefetch_flag = 1;
            L2prefetch_hits++;
            break;
        }
    }

    if (!prefetch_flag) {
        // Add to cache
        L2prefetch_misses++;
        int prefetch_empty_placement = 0;
        for (int x = 0; x < l2_assoc; x++) {
            if (matrixL2[prefetch_index][x].tag == 0 || !matrixL2[prefetch_index][x].valid) {
                matrixL2[prefetch_index][x].tag = prefetch_address.tag;
                matrixL2[prefetch_index][x].addr = (prefetch_index << block_size);
                matrixL2[prefetch_index][x].valid = 1;
                prefetch_empty_placement = 1;
                break;
            }
        }
        // Replace if line is full
        if (!prefetch_empty_placement) {
            if (replacement_policy == LRU) {
                lruFunctionL2((prefetch_index << block_size), prefetch_address.tag, prefetch_index);
            }
            else if (replacement_policy == FIFO) {
                fifoFunctionL2(prefetch_address.tag, prefetch_index, (prefetch_index << block_size));
            }
            else if (replacement_policy == OPTIMAL) {
                optimalFunctionL2((prefetch_index << block_size), prefetch_address.tag, prefetch_index);
            }
        }
    }
#endif

    // Check if read or write hit
    if(operation == 'r'){
        countReadL2++;
    }
    if(operation == 'w'){
        countWriteL2++;
    }
#if DEBUG
    printf("L2 operation %c:\n", operation);
    printf("%d: L2(index: %d, ",totalCount, index);
    printf("tag: %x) addr %x\n", tag,addr);
    printf("tag2: %x addr %x\n", tag,addr);
    for (int i = 0; i < l2_assoc; i++) {
        printf("matrix %d L2: %x\n", (i + 1), matrixL2[index][i].tag);
    }
#endif
    // Check to see if block exists
    int flag = 0;
    for(int x = 0; x < l2_assoc; x++){
        if(matrixL2[index][x].valid && matrixL2[index][x].tag == tag){
            flag = 1;
            break;
        }
    }

    // If block is found
    if(flag){
        // Read
        if(operation == 'r'){
            readHitL2++;
            // If LRU update replacement count
            if(replacement_policy == LRU){
                for(int x = 0; x < l2_assoc; x++){
                    if(matrixL2[index][x].valid && matrixL2[index][x].tag == tag){
                        matrixL2[index][x].replacementCount = 0;
                    }
                    else{
                        matrixL2[index][x].replacementCount += 1;
                    }
                }
            }
        }
        // Write
        if(operation == 'w'){
            writeHitL2++;
            // Mark as dirty
            for(int x = 0; x < l2_assoc; x++){
                if(matrixL2[index][x].valid && matrixL2[index][x].tag == tag){
                    matrixL2[index][x].dirty = 'D';
                }
            }
            // If LRU update replacement count
            if(replacement_policy == LRU){
                for(int x = 0; x < l2_assoc; x++){
                    if(matrixL2[index][x].valid && matrixL2[index][x].tag == tag){
                        matrixL2[index][x].replacementCount = 0;
                    }
                    else{
                        matrixL2[index][x].replacementCount += 1;
                    }
                }
            }
        }
    } else {
        // Add to cache. Check to see if a open block and just place it
        int emptyPlacement = 0;
        for(int x = 0; x < l2_assoc; x++){
            if(matrixL2[index][x].tag == 0 || !matrixL2[index][x].valid) {
                matrixL2[index][x].tag = tag;
                matrixL2[index][x].addr = addr;
                matrixL2[index][x].valid = 1;

                // If FIFO, update replacement count.
                if(replacement_policy == FIFO){
                    matrixL2[index][x].replacementCount = fifoCount++;
                } else if (replacement_policy == OPTIMAL) {
                    optimal_set.clear(optimal_set.list);
                }

                emptyPlacement = 1;
                break;
            }
        }
        // Replace if line is full with respective property
        if(!emptyPlacement){
            if(replacement_policy == LRU){
                lruFunctionL2(addr, tag, index);
            } else if(replacement_policy == FIFO){
                fifoFunctionL2(tag, index, addr);
            } else if (replacement_policy == OPTIMAL) {
                optimalFunctionL2(addr, tag, index);
            }

        } else{
            // Update all blocks replacement count in the row if LRU
            if(replacement_policy == LRU){
                for(int x = 0; x < l2_assoc; x++){
                    if(matrixL2[index][x].valid && matrixL2[index][x].tag == tag){
                        matrixL2[index][x].replacementCount = 0;
                    }
                    else{
                        matrixL2[index][x].replacementCount += 1;
                    }
                }
            }
        } 
        // Check if read miss and update to clean
        if(operation == 'r'){
            readMissL2++;
            for(int x = 0; x < l2_assoc; x++){
                if(matrixL2[index][x].tag == tag){
                    matrixL2[index][x].dirty = ' ';
                }
            }
        }
        // Check if write miss and update to dirty
        if(operation == 'w'){
            writeMissL2++;
            for(int x = 0; x < l2_assoc; x++){
                if(matrixL2[index][x].tag == tag){
                    matrixL2[index][x].dirty = 'D';
                }
            }
        }
    }
}

void fifoFunction(uint tag, int index, uint addr){
    // FIFO function for L1. Find the smallest number and replace with new block.
    int smallest = 9999999;
    int smallestIndex = 0;
    for(int x = 0; x < l1_assoc; x++){
        if(matrix[index][x].replacementCount < smallest){
            smallest = matrix[index][x].replacementCount;
            smallestIndex = x;
        }
    }
    if(matrix[index][smallestIndex].dirty == 'D'){
        writeback++;
        l2Cache('w',matrix[index][smallestIndex].addr);
    }
    matrix[index][smallestIndex].tag = tag;
    matrix[index][smallestIndex].addr = addr;
    matrix[index][smallestIndex].replacementCount = fifoCount++;
}

void lruFunction(uint tag, int index, uint addr){
    // LRU function for L1. Find the biggest number and replace with new block.
    int biggest = -1;
    int biggestIndex = 0;
    for(int x = 0; x < l1_assoc; x++){
        if(matrix[index][x].replacementCount > biggest && matrix[index][x].tag != 0){
            biggest = matrix[index][x].replacementCount;
            biggestIndex = x;
        }
    }
    if(matrix[index][biggestIndex].dirty == 'D'){
        writeback++;
        l2Cache('w',matrix[index][biggestIndex].addr);
    }
    matrix[index][biggestIndex].tag = tag;
    matrix[index][biggestIndex].addr = addr;
    matrix[index][biggestIndex].replacementCount = 0;
    for(int x = 0; x < l1_assoc; x++){
        if(matrix[index][x].tag != tag){
            matrix[index][x].replacementCount += 1;
        }
    }
}

void optimalFunction(uint tag, int index, uint addr) {
    // optimal function for L1. Find the optimal victim.
    uint victim;
    for (int i = 0; i < l1_assoc; i++) {
        if (matrix[index][i].valid) {
            optimal_set.push(optimal_set.list, matrix[index][i].addr);
        }
    }
    victim = optimal_victim(1);
    for (int i = 0; i < l1_assoc; i++) {
        if (matrix[index][i].addr == victim) {
            if (matrix[index][i].dirty == 'D') {
                writeback++;
                l2Cache('w', matrix[index][i].addr);
            }
            matrix[index][i].tag = tag;
            matrix[index][i].addr = addr;
            matrix[index][i].replacementCount = 0;
            matrix[index][i].valid = 1;
            optimal_set.clear(optimal_set.list);
            break;
        }
    }
}

void l1Cache(char operation, uint addr){
    // Calculate index, tag and offset for L2
    Address tmp = calc_addressing(addr, 1);
    int index = tmp.index, tag = tmp.tag;

#if OPT
    if (operation == 'r') {
        // Stride prefetching
        Address prefetchAddr = calc_addressing(addr + 1, 1); // Change to change size of the prefetch stride
        int prefetchIndex = prefetchAddr.index, prefetchTag = prefetchAddr.tag;

        // Check if the prefetch address is already in cache
        int prefetched = 0;
        for (int x = 0; x < l1_assoc; x++) {
            if (matrix[prefetchIndex][x].valid && matrix[prefetchIndex][x].tag == prefetchTag) {
                prefetched = 1;
                L1prefetch_hits++;
                break;
            }
        }

        if (!prefetched) {
            L1prefetch_misses++;
            int emptyPlacement = 0;
            for (int x = 0; x < l1_assoc; x++) {
                if (matrix[prefetchIndex][x].tag == 0 || !matrix[prefetchIndex][x].valid) {
                    matrix[prefetchIndex][x].tag = prefetchTag;
                    matrix[prefetchIndex][x].addr = addr;
                    matrix[prefetchIndex][x].valid = 1;
                    if (replacement_policy == FIFO) {
                        matrix[prefetchIndex][x].replacementCount = fifoCount++;
                    }
                    emptyPlacement = 1;
                    break;
                }
            }

            if (!emptyPlacement) {
                if (replacement_policy == LRU){
                    lruFunction(prefetchTag, prefetchIndex, addr);
                } else if (replacement_policy == FIFO){
                    fifoFunction(prefetchTag, prefetchIndex, addr);
                } else if (replacement_policy == OPTIMAL) {
                    optimalFunction(prefetchTag, prefetchIndex, addr);
                }
            }
        }
    }
#endif

    // Check if read or write hit
    if(operation == 'r'){
        countRead++;
    }
    if(operation == 'w'){
        countWrite++;
    }
#if DEBUG
    printf("%d: (index: %d, ",totalCount, index);
    printf("tag: %x)\n", tag);
    printf("matrix 1: %x\n", matrix[index][0].tag);
    printf("matrix 2: %x\n", matrix[index][1].tag);
#endif
    // Check to see if block exists
    int flag = 0;
    for(int x = 0; x < l1_assoc; x++){
        if(matrix[index][x].valid && matrix[index][x].tag == tag){
            flag = 1;
        }
    }
    // If block is found
    if(flag){
        // Read
        if(operation == 'r'){
            readHit++;
            // If LRU update replacement count
            if(replacement_policy == LRU){
                for(int x = 0; x < l1_assoc; x++){
                    if(matrix[index][x].valid && matrix[index][x].tag == tag){
                        matrix[index][x].replacementCount = 0;
                    }
                    else{
                        matrix[index][x].replacementCount += 1;
                    }
                }
            }
        }
        // Check if write hit
        if(operation == 'w'){
            writeHit++;
            // Mark as dirty
            for(int x = 0; x < l1_assoc; x++) {
                if(matrix[index][x].valid && matrix[index][x].tag == tag){
                    matrix[index][x].dirty = 'D';
                }
            }
            // If LRU update replacement count
            if(replacement_policy == LRU) {
                for(int x = 0; x < l1_assoc; x++){
                    if(matrix[index][x].valid && matrix[index][x].tag == tag){
                        matrix[index][x].replacementCount = 0;
                    }
                    else{
                        matrix[index][x].replacementCount += 1;
                    }
                }
            }
        }
    }
    else{
        // Add to cache. Check to see if a open block and just place it
        int emptyPlacement = 0;
        for(int x = 0; x < l1_assoc; x++){
            if(matrix[index][x].tag == 0 || !matrix[index][x].valid) {
                matrix[index][x].tag = tag;
                matrix[index][x].addr = addr;
                matrix[index][x].valid = 1;
                // If FIFO, update replacement count.
                if(replacement_policy == FIFO){
                    matrix[index][x].replacementCount = fifoCount++;
                } else if (replacement_policy == OPTIMAL) {
                    optimal_set.clear(optimal_set.list);
                }
                emptyPlacement = 1;
                break;
            }
        }
        // Replace if line is full with respective property
        if(!emptyPlacement) {
            if (replacement_policy == LRU){
                lruFunction(tag, index, addr);
            } else if(replacement_policy == FIFO){
                fifoFunction(tag, index, addr);
            } else if (replacement_policy == OPTIMAL) {
                optimalFunction(tag, index, addr);
            }
        } else{
            // Update all blocks replacement count in the row if LRU
            if(replacement_policy == LRU){
                for(int x = 0; x < l1_assoc; x++){
                    if(matrix[index][x].tag == tag){
                        matrix[index][x].replacementCount = 0;
                    }
                    else{
                        matrix[index][x].replacementCount += 1;
                    }
                }
            }
        }
        // Check if read miss and update to clean
        if(operation == 'r'){
            readMiss++;
            for(int x = 0; x < l1_assoc; x++){
                if(matrix[index][x].tag == tag){
                    matrix[index][x].dirty = ' ';
                    l2Cache('r',addr);
                }
            }
        }
        // Check if write miss and update to dirty
        if(operation == 'w'){
            writeMiss++;
            for(int x = 0; x < l1_assoc; x++){
                if(matrix[index][x].tag == tag){
                    matrix[index][x].dirty = 'D';
                    l2Cache('r',addr);
                }
            }
        }
    }
}

Address calc_addressing(uint addr, int lvl) {
    Address tmp;
    int offset_size = log2(block_size), index_size = 0;
    uint tag, index;
    
    tmp.addr = addr & mask;

    if (lvl == 1)
        index_size = index_size_l1;
    else if (lvl == 2)
        index_size = index_size_l2;
    addr >>= offset_size;
    tmp.index = (addr & ((1 << index_size) - 1));
    addr >>= index_size;
    tmp.tag = (addr & ((1 << (32 - index_size - offset_size)) - 1));

    return tmp;
}

void resize(ArrayList *array_list) {
    array_list->ar = realloc(array_list->ar, sizeof(uint) * (array_list->cap << 1));
    
    if (array_list->ar == NULL) {
        printf("Allocation Error in ArrayList resizing\n");
        exit(1);
    }

    array_list->cap <<= 1;
}

void append(ArrayList *array_list, uint addr) {
    if (array_list->size == array_list->cap)
        resize(array_list);
    array_list->ar[array_list->size++] = addr;
}

void trim(ArrayList *array_list) {
    array_list->ar = realloc(array_list->ar, sizeof(uint) * array_list->size);

    if (array_list->ar == NULL) {
        printf("Allocation Error in trimming\n");
        exit(1);
    }

    array_list->cap = array_list->size;
}

void delete(ArrayList *array_list, int index) {
    if (index >= array_list->size)
        return;
    
    for (int new = index, old = index + 1 ; old < array_list->size; new++, old++) {
        array_list->ar[new] = array_list->ar[old];
    }
    array_list->size--;
}

void clear(ArrayList *array_list) {
    if (array_list->size == 0)
        return;

    free(array_list->ar);
    array_list->size = 0;
    array_list->cap = l1_assoc;

    array_list->ar = malloc(sizeof(uint) * l1_assoc);
}

void printFile(FILE *trace_file_open) {
    char operation;
    uint addr;

    while (fscanf(trace_file_open, "%c %08x ", &operation, &addr) != EOF) {
        memory_addresses.push(memory_addresses.list, addr);
    }
    fseek(trace_file_open, 0, SEEK_SET);
    memory_addresses.trim(memory_addresses.list);

    fscanf(trace_file_open, "%c %08x ", &operation, &addr);

    while (!feof(trace_file_open))
    {
        //printf ("%c %08x\n",operation, i & (0xfffffff0));
        totalCount++;
        l1Cache(operation, addr & mask);
        fscanf(trace_file_open,"%c %08x ", &operation, &addr);
    }
    l1Cache(operation, addr & mask);
    printResults();
}
