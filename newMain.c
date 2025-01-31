#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

struct Node {
    char *type;
    int uniqueID;
    int *inputIDs; 
    int numInputs; 
    int *outputIDs;
    int numOutputs;
    int charge; 
};

struct Node* createNode(int uniqueID, const char *type, int *inputIDs, int *outputIDs, int numInputs, int numOutputs) {
    struct Node *node = (struct Node*)malloc(sizeof(struct Node));
    node->uniqueID = uniqueID;
    node->type = strdup(type);
    node->inputIDs = (int*)malloc(numInputs * sizeof(int));
    memcpy(node->inputIDs, inputIDs, numInputs*sizeof(int));
    node->numInputs = numInputs;
    node->outputIDs = (int*)malloc(numOutputs * sizeof(int));
    memcpy(node->outputIDs, outputIDs, numOutputs * sizeof(int));
    node->numOutputs = numOutputs;
    node->charge = 0;
    return node;
}

struct Graph { 
    struct Node **nodeArray;
    int numNodes;
    int numInputTransistors;
    int *inputIDArray;
};

struct Graph* createGraph(struct Node **nodeArray, int numNodes, int numInputTransistors, int *inputIDArray){
    struct Graph *graph = (struct Graph*)malloc(sizeof(struct Graph));
    graph->nodeArray = nodeArray;
    graph->numNodes = numNodes;
    graph->numInputTransistors = numInputTransistors;
    graph->inputIDArray = (int*)malloc(numInputTransistors * sizeof(int)); 
    memcpy(graph->inputIDArray, inputIDArray, numInputTransistors * sizeof(int));
    return graph;
}

void freeNode(struct Node *node) {
    free(node->type);
    free(node->inputIDs);
    free(node->outputIDs);
    free(node);
}

void freeGraph(struct Graph *graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        freeNode(graph->nodeArray[i]);
    }
    free(graph->inputIDArray);
    free(graph);
}

int *parseIDList(char *str, int*count) {
    *count = 0; 
    if (str == NULL || strlen(str) == 0) return NULL;
    for (int i = 0; str[i]; i++) {
        if (str[i] == ','){
            (*count)++;
        } 
    }
    if (str[strlen(str) - 1] != ',') {
        (*count)++;
    }
    int *ids = (int*)malloc((*count) * sizeof(int));
    int idx = 0;
    char *token = strtok(str, ",");
    while (token) {
        if (strlen(token) > 0) {
            ids[idx++] = atoi(token);
        }
        token = strtok(NULL, ",");
    }
    return ids;
}

struct Graph* parseFile(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }
    struct Node **nodeArray = NULL;
    int numNodes = 0;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file)){
        if (buffer[0] == '{') continue;
        char type[10];
        int uniqueID;
        char inputString[256] = "";
        char outputString[256] = "";
        if (sscanf(buffer, " Type=%s", type) == 1) {
            fgets(buffer, sizeof(buffer), file);
            sscanf(buffer, " UniqueID=%d", &uniqueID);
            fgets(buffer, sizeof(buffer), file);
            sscanf(buffer, " Input=%s", inputString);
            fgets(buffer, sizeof(buffer), file);
            sscanf(buffer, " Output=%s", outputString);
            int numInputs, numOutputs;
            int *intputIDs = parseIDList(inputString, &numInputs);
            int *outputIDs = parseIDList(outputString, &numOutputs);
            struct Node *newNode = createNode(uniqueID, type, intputIDs, outputIDs, numInputs, numOutputs);
            nodeArray = (struct Node**)realloc(nodeArray, (numNodes + 1) * sizeof(struct Node*));
            nodeArray[numNodes++] = newNode;
        }
    }
    fclose(file);
    int *inputIDArray = (int*)malloc(numNodes * sizeof(int));
    int numInputTransistors = 0;
    for (int i = 0; i < numNodes; i++){
        if (strcmp(nodeArray[i]->type, "INPUT") == 0) {
            inputIDArray[numInputTransistors++] = nodeArray[i] -> uniqueID;
        }
    }
    struct Graph *graph = createGraph(nodeArray, numNodes, numInputTransistors, inputIDArray);
    return graph;
}

int compareNodes(const void *a, const void *b) {
    struct Node *nodeA = *(struct Node **)a;
    struct Node *nodeB = *(struct Node **)b;
    return nodeA->uniqueID - nodeB->uniqueID;
}

void sortNodes(struct Graph *graph) {
    qsort(graph->nodeArray, graph->numNodes, sizeof(struct Node *), compareNodes);
}

struct Node* getNode(struct Graph *graph, int uniqueID) {
    for (int i = 0; i < graph->numNodes; i++){
        if (graph->nodeArray[i]->uniqueID == uniqueID) {
            return graph->nodeArray[i];
        }
    }
    return NULL;
}

int dfs(struct Graph *graph, struct Node *node, int *vals) {
    if (strcmp(node->type, "INPUT") == 0) {
        return vals[node->uniqueID];
    }
    if (strcmp(node->type, "OUTPUT") == 0) {
        struct Node *inputNode = getNode(graph, node->inputIDs[0]);  // OUTPUT has one input
        int result = dfs(graph, inputNode, vals);
        return result;
    }
    int *inputVals = (int*)malloc(node->numInputs * sizeof(int));
    for (int i = 0; i < node->numInputs; i++) {
        struct Node *inputNode = getNode(graph, node->inputIDs[i]);
        inputVals[i] = dfs(graph, inputNode, vals);
    }
    int result = 0;
    if (strcmp(node->type, "AND") == 0) {
        result = 1;
        for (int i = 0; i < node->numInputs; i++) {
            result &= inputVals[i];
        }
    } else if (strcmp(node->type, "OR") == 0) {
        result = 0;
        for (int i = 0; i < node->numInputs; i++) {
            result |= inputVals[i];
        }
    } else if (strcmp(node->type, "XOR") == 0) {
        result = 0;
        for (int i = 0; i < node->numInputs; i++) {
            result ^= inputVals[i];
        }
    } else if (strcmp(node->type, "NOT") == 0) {
        result = !inputVals[0];  // NOT has only one input
    }
    free(inputVals);
    return result;
}

int compareInts(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void sortInputIDs(struct Graph *graph) {
    qsort(graph->inputIDArray, graph->numInputTransistors, sizeof(int), compareInts);
}

void sortOutputIDs(struct Graph *graph, int *outputIDs, int numOutputs) {
    qsort(outputIDs, numOutputs, sizeof(int), compareInts);
}

void printTruthTable(struct Graph *graph) {
    int numInputs = graph->numInputTransistors;
    int totalCombinations = 1 << numInputs;  // 2^numInputs
    sortNodes(graph);
    sortInputIDs(graph);
    int numOutputs = 0;
    int outputIDs[graph->numNodes];  
    for (int i = 0; i < graph->numNodes; i++) {
        if (strcmp(graph->nodeArray[i]->type, "OUTPUT") == 0) {
            outputIDs[numOutputs++] = graph->nodeArray[i]->uniqueID;
        }
    }
    sortOutputIDs(graph, outputIDs, numOutputs);
    for (int i = 0; i < numInputs; i++) {
        printf("%d ", graph->inputIDArray[i]);
    }
    printf("| ");
    for (int i = 0; i < graph->numNodes; i++) {
        if (strcmp(graph->nodeArray[i]->type, "OUTPUT") == 0) {
            printf("%d ", graph->nodeArray[i]->uniqueID);
        }
    }
    printf("\n");
    for (int i = 0; i < totalCombinations; i++) {
        int inputValues[graph->numNodes];
        for (int j = 0; j < numInputs; j++) {
            inputValues[graph->inputIDArray[j]] = (i >> j) & 1;
            printf("%d ", inputValues[graph->inputIDArray[j]]);
        }
        printf("| ");
        for (int j = 0; j < graph->numNodes; j++) {
            if (strcmp(graph->nodeArray[j]->type, "OUTPUT") == 0) {
                int outputVal = dfs(graph, graph->nodeArray[j], inputValues);
                printf("%d ", outputVal);
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Missing input file argument.\n");
        return 1;
    }
    struct Graph *graph = parseFile(argv[1]);
    if (!graph) {
        fprintf(stderr, "Error: Failed to parse the input file.\n");
        return 1;
    }
    sortNodes(graph);
    printTruthTable(graph);
    freeGraph(graph);
    return 0;
}
