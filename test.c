#include <stdio.h>
#include <stdlib.h>
#include "Tries.c"
#include <string.h>

// ... (your trie structure and initialization functions)

// Define a maximum length for the path (adjust as needed)
#define MAX_PATH_LENGTH 256

// Helper function to store paths with all values from the linked list at the end
void storePathsFromNode(TriesPtr tp, const char* currentPath, LLHeadPtr allValuePaths) {
    if (tp == NULL) {
        return;
    }

    // If the current node is an end node and the linked list has values
    if (tp->is_end && tp->ports->currSize > 0) {
        // Store the full path along with all values in the linked list
        char fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, MAX_PATH_LENGTH, "%s", currentPath);

        LLNodePtr temp = tp->ports->first;
        while (temp != NULL) {
            // Append each value to the path and store it in the linked list
            char fullPathWithValue[MAX_PATH_LENGTH];
            snprintf(fullPathWithValue, MAX_PATH_LENGTH, "%s%d", fullPath, temp->val);

            LLNodePtr ln = createNode(atoi(fullPathWithValue));
            insertfront(allValuePaths, ln);

            temp = temp->next;
        }
    }

    // Recursively store paths for each child node
    for (int i = 0; i < 128; i++) {
        if (tp->alpha[i] != NULL) {
            char newPath[MAX_PATH_LENGTH];
            snprintf(newPath, MAX_PATH_LENGTH, "%s%c", currentPath, (char)i);
            storePathsFromNode(tp->alpha[i], newPath, allValuePaths);
        }
    }
}

// Function to store paths with all values from the linked list at the end
LLHeadPtr storeAllValuePathsFromTriesPtr(TriesPtr tp) {
    LLHeadPtr allValuePaths = initList();
    storePathsFromNode(tp, "", allValuePaths);
    return allValuePaths;
}

int main() {
    // Example usage
    TriesHeadPtr thp = initTries();

    // Insert elements into the trie
    insertTries3(thp, "dir1/dir2/f1", 1, 2, 3);
    insertTries3(thp, "dir1/dir3/f2", 4, 5, 6);
    insertTries3(thp, "dir1/dir2/f2", 7, 8, 9);
    insertTries3(thp, "dir1/f4", 10, 11, 12);  // Added a path with values at the end

    printTries(thp->head, 0);

    // Assuming you have a specific TriesPtr, for example, thp->head->alpha['d']->alpha['i']->alpha['r']
    TriesPtr specificTriesPtr = thp->head->alpha['d']->alpha['i']->alpha['r'];

    // Store paths with all values from the linked list at the end
    LLHeadPtr allValuePaths = storeAllValuePathsFromTriesPtr(specificTriesPtr);

    // Print the stored paths
    printf("Paths with all values at the end:\n");
    printList(allValuePaths);

    // Clean up resources (implement your cleanup functions)
    // ...

    return 0;
}
