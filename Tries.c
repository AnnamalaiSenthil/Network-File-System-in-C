#include "Tries.h"

LL_NODE_TRIE_CONTENT init_LL_Trie()
{
    LL_NODE_TRIE_CONTENT temp=(LL_NODE_TRIE_CONTENT)malloc(sizeof(struct LL_node_trie_content));
    temp->next=NULL;
    temp->val=malloc(sizeof(char)*512);
    temp->ss_index=0;
}

TRIE_CONTENT_HEAD init_LL_Trie_Head()
{
    TRIE_CONTENT_HEAD T=(TRIE_CONTENT_HEAD)malloc(sizeof(struct Trie_content_Head));
    T->first=NULL;
}

void InsertLLnode_trie(TRIE_CONTENT_HEAD LLH,char * val,int port)
{
    LL_NODE_TRIE_CONTENT T=init_LL_Trie();
    T->ss_index=port;
    strcpy(T->val,val);
    T->next=LLH->first;
    LLH->first=T;
}


// to insert element into the front of the linked list
void insertfront(LLHeadPtr qn, LLNodePtr elem)
{
    if (qn->currSize == 0)
    {
        qn->first = elem;
        qn->currSize = 1;
    }
    else
    {
        elem->next = qn->first;
        qn->first = elem;
        qn->currSize = 1 + qn->currSize;
    }
}

// to initialize the linkedlist
LLHeadPtr initList()
{
    LLHeadPtr lh = (LLHeadPtr)malloc(sizeof(struct LLHead));
    lh->currSize = 0;
    lh->first = NULL;
    return lh;
}

// to create a node of the linkedList
LLNodePtr createNode(int val)
{
    LLNodePtr ln = (LLNodePtr)malloc(sizeof(struct LLNode));
    ln->next = NULL;
    ln->val = val;
    return ln;
}

// to print the elements of the linked list
void printList(LLHeadPtr lh)
{
    LLNodePtr ln = lh->first;
    while (ln != NULL)
    {
        printf("%d ", ln->val);
        ln = ln->next;
    }
    printf("\n");
}

// initiate each node of the tries
TriesPtr initTrieNode()
{
    TriesPtr tp = (TriesPtr)malloc(sizeof(struct Tries));
    tp->alpha = (TriesPtr *)malloc(sizeof(TriesPtr) * 128);
    tp->count = 0;
    tp->ports = initList();
    tp->is_end = 0;
    for (int i = 0; i < 128; i++)
    {
        tp->alpha[i] = NULL;
    }
    return tp;
}
// to intitiate the whole tries
TriesHeadPtr initTries()
{
    TriesHeadPtr thp = (TriesHeadPtr)malloc(sizeof(struct TriesHead));
    thp->head = initTrieNode();
    return thp;
}
// a function to check if this value already exists or not
int checkForPortExistance(LLHeadPtr ll, int port)
{
    // assuming that the value doesnt exist
    LLNodePtr temp = ll->first;
    while (temp != NULL)
    {
        if (temp->val == port)
        {
            return 0;
        }
        temp = temp->next;
    }
    return 1;
}
// this is to insert elements into the tries, so for a particular value which port of server is available
void insertTries3(TriesHeadPtr thp, char *val, int ss1, int ss2, int ss3)
{
    TriesPtr tp = thp->head;
    int length = strlen(val);
    for (int i = 0; i <= length; i++)
    {
        // printf("%d*",tp->count);
        tp->count++;
        // tp->is_end = 0;
        if (i != length)
        {
            int pos = (int)val[i];
            if (tp->alpha[pos] == NULL)
            {
                tp->alpha[pos] = initTrieNode();
            }
            tp = tp->alpha[pos];
        }
    }
    // printf("\n");
    tp->is_end = 1;
    if (checkForPortExistance(tp->ports, ss1))
    {
        LLNodePtr ln = createNode(ss1);
        insertfront(tp->ports, ln);
    }
    if (checkForPortExistance(tp->ports, ss2))
    {
        LLNodePtr ln = createNode(ss2);
        insertfront(tp->ports, ln);
    }
    if (checkForPortExistance(tp->ports, ss3))
    {
        LLNodePtr ln = createNode(ss3);
        insertfront(tp->ports, ln);
    }

    // printList(tp->ports);
    // printf("%lld\n", tp->ports->currSize);
}
// this is to insert elements into the tries, so for a particular value which port of server is available
void insertTries1(TriesHeadPtr thp, char *val, int ss1)
{
    TriesPtr tp = thp->head;
    int length = strlen(val);
    for (int i = 0; i < length; i++)
    {
        int pos = (int)val[i];
        // printf("In loop %d  %d  %p\n", i,pos,tp);
        tp->is_end = 0;
        if (tp->alpha[pos] == NULL)
        {
            tp->alpha[pos] = initTrieNode();
        }
        // LLNodePtr ln = createNode(port);
        // insertfront(tp->ports, ln);
        tp = tp->alpha[pos];
    }
    tp->is_end = 1;
    if (checkForPortExistance(tp->ports, ss1))
    {
        LLNodePtr ln = createNode(ss1);
        insertfront(tp->ports, ln);
    }
    // printList(tp->ports);
    // printf("%lld\n", tp->ports->currSize);
}

void searchAll(TriesPtr tp, LLHeadPtr lh)
{
    for (int i = 0; i < 128; i++)
    {

        if (tp->alpha[i] != NULL)
        {
            // printf("index is  %d \n",i);
            if (tp->alpha[i]->ports->first != NULL)
            {
                LLNodePtr temp = tp->alpha[i]->ports->first;
                while (temp != NULL)
                {
                    // printf("%s\n", temp->val);
                    LLNodePtr nN = createNode(temp->val);
                    insertfront(lh, nN);
                    temp = temp->next;
                }
            }
            searchAll(tp->alpha[i], lh);
        }
    }
}

int normalCompare(const void *stringOne, const void *stringTwo)
{
    return strcmp(*(const char **)stringOne, *(const char **)stringTwo);
}

// this function searches for a particular file and tells us if it has content or not, and puts it into a list
void searchWords(TriesHeadPtr thp, char *val, LLHeadPtr lh)
{
    TriesPtr tp = thp->head;
    for (int i = 0; val[i] != '\0'; i++)
    {
        int pos = (int)val[i];
        // printf("pos is %d\n", pos);
        if (tp->alpha[pos] == NULL)
        {
            return;
        }
        tp = tp->alpha[pos];
    }
    if (tp->ports->first != NULL)
    {
        LLNodePtr temp = tp->ports->first;
        while (temp != NULL)
        {
            LLNodePtr nN = createNode(temp->val);
            insertfront(lh, nN);
            temp = temp->next;
        }
    }
    if(lh->currSize==0)
    {
        lh->currSize=-1;
    }
    // searchAll(tp, lh);
}

void insertback(LLHeadPtr qn, LLNodePtr elem)
{
    if (qn->currSize == 0)
    {
        qn->first = elem;
        elem->next = NULL;
        qn->currSize = 1;
    }
    else if (qn->currSize == 1)
    {
        qn->first->next = elem;
        elem->next = NULL;
        qn->currSize = 2;
    }
    else
    {
        qn->first->next->next = elem;
        elem->next = NULL;
        qn->currSize = 3;
    }
}

void printTries(TriesPtr tp, int j)
{
    for (int i = 0; i < 128; i++)
    {
        if (tp->alpha[i] != NULL)
        {
            for (int k = 0; k < j; k++)
            {
                printf(" ");
            }
            printf("%c ", i);
            printList(tp->alpha[i]->ports);

            printTries(tp->alpha[i], j + 1);
        }
    }
}

void printTriesFile(TriesPtr tp, int j,FILE *file,char *val)
{
    for (int i = 0; i < 128; i++)
    {
        if (tp->alpha[i] != NULL)
        {
            
            // printf("%c ", i);
            val[j]=(char)i;
            val[j+1]='\0';
            if(tp->alpha[i]->is_end==1)
            {
                // for (int k = 0; k < q; k++)
                // {
                //     fprintf(file,"\t");
                //     printf("\t");
                // }
                fprintf(file,"%s\n",val);
                printf("%s\n",val);
            }
            // printList(tp->alpha[i]->ports);

            printTriesFile(tp->alpha[i], j + 1,file,val);
        }
    }
}


 // j parameter should be the length of the initial string provided that is val
void Tries_Ports_fn(TRIE_CONTENT_HEAD LLH, TriesPtr tp, int j,char *val)   
{
    for (int i = 0; i < 128; i++)
    {
        if (tp->alpha[i] != NULL)
        {
            
            // printf("%c ", i);
            val[j]=(char)i;
            val[j+1]='\0';
            if(tp->alpha[i]->is_end==1)
            {
                // LL_NODE_TRIE_CONTENT T=initLL_Trie();
                InsertLLnode_trie(LLH,val,tp->alpha[i]->ports->first->val);
                // printf("%s\n",val);
            }
            Tries_Ports_fn(LLH,tp->alpha[i], j + 1,val);
        }
    }
}

TRIE_CONTENT_HEAD LL_Tries_Ports_fn(TriesHeadPtr THP,char * val)
{
    TRIE_CONTENT_HEAD LLH=init_LL_Trie_Head();
    TriesPtr T=THP->head;
    for(int i=0;i<strlen(val);i++)
    {
        if(T->alpha[val[i]]==NULL)
        {
            return LLH;
        }
        T=T->alpha[val[i]];
    }
    if(T->alpha['/']!=NULL)
        T=T->alpha['/'];
    else
        return LLH;
    int j=strlen(val);
    val[j]='/';
    val[j+1]='\0';
    Tries_Ports_fn(LLH,T,j+1,val);
    return LLH;
}

char *rec(TriesPtr tp, char *val, int len)
{
    // char * T=(char *)malloc(sizeof(char)*512);
    for (int i = 0; i < 128; i++)
    {
        if (tp->alpha[i] != NULL)
        {
            val[len] = (char)i;
            val[len + 1] = '\0';
            if (tp->alpha[i]->is_end == 1)
                return val;
            return rec(tp->alpha[i], val, len + 1);
        }
    }
    return val;
}

char *nextentry(TriesHeadPtr thp, char *val)
{
    TriesPtr tp = thp->head;
    int length = strlen(val);
    for (int i = 0; i < length; i++)
    {
        if (i != length)
        {
            int pos = (int)val[i];
            if (tp->alpha[pos] == NULL)
                return "";
            tp = tp->alpha[pos];
        }
    }
    char *T = (char *)malloc(sizeof(char) * 512);
    strcpy(T, val);
    return rec(tp, T, length);
}

void saveTrieContentsToFile(TriesPtr tp, char *word, FILE *file)
{
    // Base case: if the current node is NULL, return
    if (tp == NULL)
    {
        return;
    }

    // If the current node is an end node, save its word and ports to the file
    if (tp->is_end)
    {
        fprintf(file, "%s Ports:", word);
        LLNodePtr temp = tp->ports->first;
        while (temp != NULL)
        {
            fprintf(file, " %d", temp->val);
            temp = temp->next;
        }
        fprintf(file, "\n");
    }

    // Recursively save trie contents for each child node
    for (int i = 0; i < 128; i++)
    {
        if (tp->alpha[i] != NULL)
        {
            char newWord[256]; // Assuming the maximum word length is 256
            strcpy(newWord, word);
            int len = strlen(newWord);
            newWord[len] = i;
            newWord[len + 1] = '\0';
            saveTrieContentsToFile(tp->alpha[i], newWord, file);
        }
    }
}

void saveTrieToFile(TriesHeadPtr thp, const char *filename)
{
    FILE *file = fopen(filename, "w"); // Open the file in write mode (overwrites existing content)

    // Check if the file is opened successfully
    if (file == NULL)
    {
        printf("Error opening the file!\n");
        return;
    }

    char rootWord[1]; // Root node has an empty word
    rootWord[0] = '\0';
    saveTrieContentsToFile(thp->head, rootWord, file);

    // Close the file after writing
    fclose(file);
}

TriesHeadPtr loadTrieContentsFromFile(const char *filename)
{
    FILE *file = fopen(filename, "r");

    // Check if the file is opened successfully
    if (file == NULL)
    {
        printf("Error opening the file!\n");
        return NULL;
    }

    TriesHeadPtr thp = initTries();

    char word[256];
    int port;
    while (fscanf(file, "%s Ports: ", word) != EOF)
    {
        TriesPtr tp = thp->head;
        int length = strlen(word);
        for (int i = 0; i < length; i++)
        {
            int pos = (int)word[i];
            if (tp->alpha[pos] == NULL)
            {
                tp->alpha[pos] = initTrieNode();
            }
            tp->count++;
            tp = tp->alpha[pos];
        }
        tp->count++;
        // Extract and add ports to the trie
        while (fscanf(file, "%d", &port) == 1)
        {
            tp->is_end = 1;
            LLNodePtr ln = createNode(port);
            insertback(tp->ports, ln);
        }

        // Consume the newline character after reading ports
        char newline;
        // fscanf(file, "%c", &newline);
    }

    // Close the file after reading
    fclose(file);

    return thp;
}

void Delete_Trie(TriesHeadPtr thp)
{
}

void DeleteNode(TriesHeadPtr thp, char *val)
{
    TriesPtr *tp = (TriesPtr *)malloc(sizeof(TriesPtr));
    *tp = (thp->head);
    int length = strlen(val);
    for (int i = 0; i <= length; i++)
    {
        (*tp)->count--;
        // printf("%d-",(*tp)->count);
        if (i != length)
        {
            int pos = (int)val[i];
            *tp = (*tp)->alpha[pos];
        }
    }
    // printf("\n");
    (*tp)->is_end=0;
    *tp = thp->head;
    for (int i = 0; i <= length; i++)
    {
        TriesPtr *T = (TriesPtr *)malloc(sizeof(struct Tries));
        *T = (*tp);
        if (i != length)
        {
            int pos = (int)val[i];
            *tp = (*tp)->alpha[pos];
            if ((*tp)->count == 0)
                (*T)->alpha[pos] = NULL;
        }
        if ((*T)->count == 0 && (*T) != thp->head)
        {
            // printf("Hello\n");
            free((*T)->alpha);
            free((*T)->ports);
            free((*T));
            free(T);
        }
    }
}
int searchSubstring(TriesPtr tp, const char *substring) {
    int length = strlen(substring);
    for (int i = 0; i < length; i++) {
        int pos = (int)substring[i];

        if (tp->alpha[pos] == NULL) {
            // Substring not found
            return 0;
        }

        tp = tp->alpha[pos];
    }

    // Substring found
    return 1;
}

// int main()
// {
//    // Initialize a new trie
//     TriesHeadPtr thp = initTries();
// //    thp = loadTrieContentsFromFile("trie_contents.txt");
//    // Insert elements into the trie
//     insertTries3(thp, "Anna1", 1,1,1);
//     insertTries3(thp, "dir3/file1", 3,3,3);
//     insertTries3(thp, "dir1/file1", 3,3,3);
//     insertTries3(thp, "Anna2", 2,2,2);
//     insertTries3(thp, "dir1/file7", 3,3,3);
//     insertTries3(thp, "dir1/file2", 5,5,5);
// //    insertTries1(thp, "", 3);
// //    insertTries1(thp, "dir5/file5", 2);
// //    insertTries1(thp, "dir5/file6", 7);
// //    insertTries1(thp, "dir5/file3", 8);
// //    insertTries1(thp, "dir3/file1", 88);
// //    insertTries1(thp, "dir3/file2", 87);
// //    insertTries1(thp, "dir3/file3", 88);
//    // Print trie contents before saving
// //    printf("Trie contents before saving to file:\n");
// //    printTries(thp->head, 0);
//     // printf("%d\n",thp->head->alpha['A']->alpha['n']->alpha['n']->alpha['a']->alpha['1']->is_end);
//     // exit(0);
//     // printf("%s\n",nextentry(thp,"Anna1"));
//     // while(thp->head->alpha['A']->alpha['n']->count>1)
//     // {
//     //     DeleteNode(thp,nextentry(thp,"An"));
//     // }
//     // printTries(thp->head, 0);
//     // exit(0);
//     // DeleteNode(thp,nextentry(thp,"An"));
//     // printf("Hi\n");
//     // printTries(thp->head, 0);
// //    DeleteNode(thp,"Anna1");
// //    printf("%s\n",nextentry(thp,"An"));

// //    printf("%d\n",thp->head->alpha['d']->alpha['i']->alpha['v']->alpha['4']->is_end);
//    // Save the entire trie to a file
// //    saveTrieToFile(thp, "trie_contents.txt");
// //    printf("Trie contents saved to file.\n");
//    // Free the allocated memory for the trie
//    // Implement a function to free the memory used by the trie and linked lists
//     char * val=(char *)malloc(sizeof(char)*512);
//     strcpy(val,"dir1/");
//     TRIE_CONTENT_HEAD LLH=LL_Tries_Ports_fn(thp,val);
//     LL_NODE_TRIE_CONTENT T=LLH->first;
//     while(T!=NULL)
//     {
//         printf("%s %d\n",T->val,T->ss_index);
//         T=T->next;
//     }
//     return 0;
// }