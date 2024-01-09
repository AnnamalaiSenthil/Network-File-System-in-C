#include "SS_functions.h"

int createFile(const char *filePath)
{
    FILE *file = fopen(filePath, "w");
    if (file != NULL)
    {
        printf("File created at: %s\n", filePath);
        fclose(file);
        return 1; // Success
    }
    else
    {
        perror("Error creating file");
        return 0; // Failure
    }
}

int createDirectory(const char *dirPath)
{
    if (mkdir(dirPath, 0777) == 0 || errno == EEXIST)
    {
        printf("Directory created: %s\n", dirPath);
        return 1; // Success
    }
    else
    {
        perror("Error creating directory");
        return 0; // Failure
    }
}

int createFilePath(const char *inputPath)
{

    char cwd[1024]; // Assuming the path won't exceed 1024 characters

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
        return EXIT_FAILURE;
    }
    char *token, *rest;
    char *pathCopy = strdup(inputPath);

    // Initial split
    token = strtok_r(pathCopy, "/", &rest);

    // Keep track of successfully created components
    char currentPath[256];

    while (token != NULL)
    {
        snprintf(currentPath, sizeof(currentPath), "%s", token);

        // Check if it's the last component in the path
        if (*rest == '\0')
        {
            // It's the last component, create the file
            if (!createFile(currentPath))
            {
                printf("ERROR HERE!!!\n");
                return 0; // Failure
            }
            else
                break;
        }
        else
        {
            // It's not the last component, create the directory
            if (!createDirectory(currentPath))
            {
                return 0; // Failure
            }
        }

        // Change into the file or directory's parent
        if (chdir(currentPath) != 0)
        {
            perror("Error changing directory");
            return 0; // Failure
        }

        // Move to the next component in the path
        token = strtok_r(NULL, "/", &rest);
    }
    printf("%s***\n", cwd);
    // Change back to the original directory
    if (chdir(cwd) != 0)
    {
        perror("Error changing back to the original directory");
        return 0; // Failure
    }

    printf("Path created successfully.\n");

    free(pathCopy);
    return 1; // Success
}
int createDirPath(const char *inputPath)
{
    char cwd[1024]; // Assuming the path won't exceed 1024 characters

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
        return EXIT_FAILURE;
    }
    char *token, *rest;
    char *pathCopy = strdup(inputPath);

    // Initial split
    token = strtok_r(pathCopy, "/", &rest);

    // Keep track of successfully created components
    char currentPath[256];

    while (token != NULL)
    {
        snprintf(currentPath, sizeof(currentPath), "%s", token);

        // Check if it's the last component in the path
        if (*rest == '\0')
        {
            // It's the last component, create it as a directory
            if (!createDirectory(currentPath))
            {
                return 0; // Failure
            }
        }
        else
        {
            // It's not the last component, create it as a directory
            if (!createDirectory(currentPath))
            {
                return 0; // Failure
            }
        }

        // Change into the directory's parent
        if (chdir(currentPath) != 0)
        {
            perror("Error changing directory");
            return 0; // Failure
        }

        // Move to the next component in the path
        token = strtok_r(NULL, "/", &rest);
    }

    // Change back to the original directory
    if (chdir(cwd) != 0)
    {
        perror("Error changing back to the original directory");
        return 0; // Failure
    }

    printf("Path created successfully.\n");

    free(pathCopy);
    return 1; // Success
}
int deleteFile(const char *filePath)
{
    if (remove(filePath) == 0)
    {
        printf("File deleted: %s\n", filePath);
        return 1; // Success
    }
    else
    {
        perror("Error deleting file");
        return 0; // Failure
    }
}



/*int removeLastDir(const char *inputPath)
{
    char cwd[1024]; // Assuming the path won't exceed 1024 characters

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
        return EXIT_FAILURE;
    }
    char *token, *rest;
    char *pathCopy = strdup(inputPath);

    // Initial split
    token = strtok_r(pathCopy, "/", &rest);

    // Keep track of successfully created components
    char currentPath[256];

    while (token != NULL)
    {
        snprintf(currentPath, sizeof(currentPath), "%s", token);

        // Check if it's the last component in the path
        if (*rest == '\0')
        {
            // It's the last component, delete it as a directory
            if (!deleteFolder(currentPath))
            {
                return 0; // Failure
            }
        }
        else
        {
            // It's not the last component, do nothing
            // Change into the directory's parent
            if (chdir(currentPath) != 0)
            {
                perror("Error changing directory");
                return 0; // Failure
            }
        }

        // Move to the next component in the path
        token = strtok_r(NULL, "/", &rest);
    }

    // Change back to the original directory
    if (chdir(cwd) != 0)
    {
        perror("Error changing back to the original directory");
        return 0; // Failure
    }

    printf("Path created successfully.\n");

    free(pathCopy);
    return 1; // Success
}
*/
int removeLastFile(const char *inputPath)
{

    char cwd[1024]; // Assuming the path won't exceed 1024 characters

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd() error");
        return EXIT_FAILURE;
    }
    char *token, *rest;
    char *pathCopy = strdup(inputPath);

    // Initial split
    token = strtok_r(pathCopy, "/", &rest);

    // Keep track of successfully created components
    char currentPath[256];

    while (token != NULL)
    {
        snprintf(currentPath, sizeof(currentPath), "%s", token);

        // Check if it's the last component in the path
        if (*rest == '\0')
        {
            // It's the last component, delete it as a directory
            if (!deleteFile(currentPath))
            {
                return 0; // Failure
            }
        }
        else
        {
            // It's not the last component, do nothing
            // Change into the directory's parent
            if (chdir(currentPath) != 0)
            {
                perror("Error changing directory");
                return 0; // Failure
            }
        }

        // Move to the next component in the path
        token = strtok_r(NULL, "/", &rest);
    }

    // Change back to the original directory
    if (chdir(cwd) != 0)
    {
        perror("Error changing back to the original directory");
        return 0; // Failure
    }

    printf("Path created successfully.\n");

    free(pathCopy);
    return 1; // Success
}