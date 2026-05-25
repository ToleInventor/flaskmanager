#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define CMD_BUFFER_SIZE 8192



int runCommandVerbose(const char *cmdLine, int showOutput)
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    char cmd[CMD_BUFFER_SIZE];

    printf("[CMD] %s\n", cmdLine);

    snprintf(cmd, sizeof(cmd), "cmd.exe /c \"%s\"", cmdLine);

    if (showOutput)
    {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOW;
        if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            return -1;
    }
    else
    {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
            return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exitCode;
}



int directoryExists(const char *path)
{
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}



int fileExists(const char *path)
{
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES);
}



int createDirectoryCmd(const char *path)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\" 2>nul", path);
    printf("Creating directory: %s\n", path);
    return runCommandVerbose(cmd, 0);
}



int removeDirectoryCmd(const char *path)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\"", path);
    printf("Removing directory: %s\n", path);
    return runCommandVerbose(cmd, 0);
}



int writeFileLines(const char *path, const char *lines[], int lineCount)
{
    char cmd[CMD_BUFFER_SIZE];

    printf("Writing file: %s\n", path);
    snprintf(cmd, sizeof(cmd), "type nul > \"%s\"", path);
    if (runCommandVerbose(cmd, 0) != 0) return -1;

    for (int i = 0; i < lineCount; i++)
    {
        if (strlen(lines[i]) == 0)
        {
            snprintf(cmd, sizeof(cmd), "echo. >> \"%s\"", path);
            if (runCommandVerbose(cmd, 0) != 0) return -1;
        }
        else
        {
            char escaped[CMD_BUFFER_SIZE];
            const char *src = lines[i];
            char *dst = escaped;
            while (*src && (dst - escaped) < (CMD_BUFFER_SIZE - 10))
            {
                if (*src == '>' || *src == '<' || *src == '|' || *src == '&' || *src == '%' || *src == '"' || *src == '(' || *src == ')')
                {
                    *dst++ = '^';
                }
                *dst++ = *src++;
            }
            *dst = '\0';
            snprintf(cmd, sizeof(cmd), "echo %s >> \"%s\"", escaped, path);
            if (runCommandVerbose(cmd, 0) != 0) return -1;
        }
    }
    return 0;
}



int readFileToString(const char *path, char *buffer, int bufferSize)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    size_t len = fread(buffer, 1, bufferSize - 1, f);
    buffer[len] = '\0';
    fclose(f);
    return 0;
}



int computeRequirementsHash(const char *projectPath, char *hashOut, int hashOutSize)
{
    char reqPath[BUFFER_SIZE];
    char cmd[CMD_BUFFER_SIZE];
    char tempFile[BUFFER_SIZE];
    snprintf(reqPath, sizeof(reqPath), "%s\\requirements.txt", projectPath);
    if (!fileExists(reqPath))
    {
        snprintf(hashOut, hashOutSize, "no_requirements");
        return 0;
    }
    GetTempPathA(BUFFER_SIZE, tempFile);
    strcat(tempFile, "flaskman_hash.tmp");
    snprintf(cmd, sizeof(cmd), "certutil -hashfile \"%s\" MD5 > \"%s\"", reqPath, tempFile);
    runCommandVerbose(cmd, 0);
    char rawHash[BUFFER_SIZE] = {0};
    readFileToString(tempFile, rawHash, sizeof(rawHash));
    remove(tempFile);
    char *newline = strchr(rawHash, '\n');
    if (newline) *newline = '\0';
    snprintf(hashOut, hashOutSize, "%s", rawHash);
    return 0;
}



void updateFlaskMan(const char *projectPath, const char *hash)
{
    char flaskmanPath[BUFFER_SIZE];
    snprintf(flaskmanPath, sizeof(flaskmanPath), "%s\\.flaskman", projectPath);
    const char *lines[] = { hash };
    writeFileLines(flaskmanPath, lines, 1);
    printf("Updated .flaskman with hash: %s\n", hash);
}



int dependenciesMatch(const char *projectPath)
{
    char flaskmanPath[BUFFER_SIZE];
    char storedHash[BUFFER_SIZE] = {0};
    char currentHash[BUFFER_SIZE] = {0};
    snprintf(flaskmanPath, sizeof(flaskmanPath), "%s\\.flaskman", projectPath);
    if (!fileExists(flaskmanPath))
        return 0;
    if (readFileToString(flaskmanPath, storedHash, sizeof(storedHash)) != 0)
        return 0;
    storedHash[strcspn(storedHash, "\n\r")] = 0;
    computeRequirementsHash(projectPath, currentHash, sizeof(currentHash));
    return (strcmp(storedHash, currentHash) == 0);
}



void installRequirements(const char *projectPath)
{
    char cmd[CMD_BUFFER_SIZE];
    char reqPath[BUFFER_SIZE];
    snprintf(reqPath, sizeof(reqPath), "%s\\requirements.txt", projectPath);
    if (!fileExists(reqPath))
    {
        printf("No requirements.txt found. Nothing to install.\n");
        return;
    }
    printf("Installing dependencies from requirements.txt...\n");
    snprintf(cmd, sizeof(cmd), "cd /d \"%s\" && venv\\Scripts\\pip install -r requirements.txt", projectPath);
    runCommandVerbose(cmd, 1);
    char newHash[BUFFER_SIZE];
    computeRequirementsHash(projectPath, newHash, sizeof(newHash));
    updateFlaskMan(projectPath, newHash);
    printf("Dependencies installation completed.\n");
}



void ensureDependenciesMatch(const char *projectPath)
{
    if (!dependenciesMatch(projectPath))
    {
        printf("Dependencies out of sync. Installing missing packages...\n");
        installRequirements(projectPath);
    }
    else
    {
        printf("Dependencies are up to date.\n");
    }
}



int checkPython(void)
{
    printf("Checking for Python...\n");
    int ret = runCommandVerbose("python --version", 1);
    return (ret == 0);
}



int checkVSCode(void)
{
    printf("Checking for Visual Studio Code...\n");
    int ret = runCommandVerbose("code --version", 1);
    return (ret == 0);
}



void installPythonChoice(void)
{
    int choice = 0;
    while (1)
    {
        printf("\nPython is missing. Choose installation option:\n");
        printf("a) Latest version via winget [WARNING: may break dependencies]\n");
        printf("b) Python 3.12 Stable via winget\n");
        printf("Choice (a/b): ");
        char input[10];
        fgets(input, sizeof(input), stdin);
        if (input[0] == 'a' || input[0] == 'A')
        {
            printf("Installing latest Python...\n");
            runCommandVerbose("winget install --id Python.Python --silent --accept-package-agreements", 1);
            break;
        }
        else if (input[0] == 'b' || input[0] == 'B')
        {
            printf("Installing Python 3.12...\n");
            runCommandVerbose("winget install --id Python.Python.3.12 --silent --accept-package-agreements", 1);
            break;
        }
        else
        {
            printf("Invalid choice.\n");
        }
    }
}



void installVSCode(void)
{
    printf("Visual Studio Code not found. Installing silently...\n");
    runCommandVerbose("winget install --id Microsoft.VisualStudioCode --silent --accept-package-agreements", 1);
}



char* getBasePath(int global)
{
    static char path[BUFFER_SIZE];
    if (global)
    {
        const char *systemDrive = getenv("SystemDrive");
        if (!systemDrive) systemDrive = "C:";
        snprintf(path, sizeof(path), "%s\\Users\\flask_manager", systemDrive);
    }
    else
    {
        const char *userProfile = getenv("USERPROFILE");
        if (!userProfile) userProfile = ".";
        snprintf(path, sizeof(path), "%s\\flask_manager", userProfile);
    }
    printf("Base directory: %s\n", path);
    return path;
}



void enumerateProjects(const char *basePath, char projects[][BUFFER_SIZE], int *count)
{
    char searchPath[BUFFER_SIZE];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", basePath);
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        *count = 0;
        return;
    }
    *count = 0;
    do
    {
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0)
        {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                snprintf(projects[*count], BUFFER_SIZE, "%s\\%s", basePath, findData.cFileName);
                (*count)++;
                if (*count >= 100) break;
            }
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
}



char* selectProject(const char *basePath)
{
    static char selected[BUFFER_SIZE] = "";
    char projects[100][BUFFER_SIZE];
    int count = 0;

    enumerateProjects(basePath, projects, &count);
    if (count == 0)
    {
        printf("No projects found.\n");
        return NULL;
    }

    printf("Available projects:\n");
    for (int i = 0; i < count; i++)
    {
        printf("%d. %s\n", i + 1, projects[i]);
    }
    printf("Enter number: ");
    int choice;
    scanf("%d", &choice);
    while (getchar() != '\n');
    if (choice < 1 || choice > count)
    {
        printf("Invalid choice.\n");
        return NULL;
    }
    strcpy(selected, projects[choice - 1]);
    return selected;
}



int createFlaskProject(const char *projectPath)
{
    char templatesPath[BUFFER_SIZE];
    char staticPath[BUFFER_SIZE];
    char appPath[BUFFER_SIZE];
    char indexPath[BUFFER_SIZE];
    char gitignorePath[BUFFER_SIZE];
    char requirementsPath[BUFFER_SIZE];
    char flaskmanPath[BUFFER_SIZE];

    snprintf(templatesPath, sizeof(templatesPath), "%s\\templates", projectPath);
    snprintf(staticPath, sizeof(staticPath), "%s\\static", projectPath);
    snprintf(appPath, sizeof(appPath), "%s\\app.py", projectPath);
    snprintf(indexPath, sizeof(indexPath), "%s\\templates\\index.html", projectPath);
    snprintf(gitignorePath, sizeof(gitignorePath), "%s\\.gitignore", projectPath);
    snprintf(requirementsPath, sizeof(requirementsPath), "%s\\requirements.txt", projectPath);
    snprintf(flaskmanPath, sizeof(flaskmanPath), "%s\\.flaskman", projectPath);

    printf("Creating Flask project structure at: %s\n", projectPath);

    if (createDirectoryCmd(projectPath) != 0) return -1;
    if (createDirectoryCmd(templatesPath) != 0) return -1;
    if (createDirectoryCmd(staticPath) != 0) return -1;

    const char *appLines[] = {
        "from flask import Flask, render_template",
        "",
        "app = Flask(__name__)",
        "",
        "@app.route('/')",
        "def home():",
        "    return render_template('index.html')",
        "",
        "if __name__ == '__main__':",
        "    app.run(debug=True)"
    };
    if (writeFileLines(appPath, appLines, sizeof(appLines)/sizeof(appLines[0])) != 0) return -1;

    const char *htmlLines[] = {
        "<!DOCTYPE html>",
        "<html>",
        "<head><title>Flask App</title></head>",
        "<body>",
        "<h1>Hello from Flask!</h1>",
        "</body>",
        "</html>"
    };
    if (writeFileLines(indexPath, htmlLines, sizeof(htmlLines)/sizeof(htmlLines[0])) != 0) return -1;

    const char *gitignoreLines[] = {
        "venv/",
        "__pycache__/",
        "*.pyc",
        ".env",
        "*.log",
        ".flaskman"
    };
    if (writeFileLines(gitignorePath, gitignoreLines, sizeof(gitignoreLines)/sizeof(gitignoreLines[0])) != 0) return -1;

    const char *requirementsLines[] = {
        "flask"
    };
    if (writeFileLines(requirementsPath, requirementsLines, sizeof(requirementsLines)/sizeof(requirementsLines[0])) != 0) return -1;

    const char *hashPlaceholder[] = { "0" };
    if (writeFileLines(flaskmanPath, hashPlaceholder, 1) != 0) return -1;

    return 0;
}



void ensureVenv(const char *projectPath)
{
    char venvPath[BUFFER_SIZE];
    char pythonCmd[CMD_BUFFER_SIZE];
    snprintf(venvPath, sizeof(venvPath), "%s\\venv", projectPath);
    if (!directoryExists(venvPath))
    {
        printf("Creating virtual environment...\n");
        snprintf(pythonCmd, sizeof(pythonCmd), "cd /d \"%s\" && python -m venv venv", projectPath);
        runCommandVerbose(pythonCmd, 1);
    }
    ensureDependenciesMatch(projectPath);
}



void runFlaskApp(const char *projectPath)
{
    printf("Checking dependencies before running...\n");
    ensureDependenciesMatch(projectPath);
    printf("Starting Flask application...\n");
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "start cmd.exe /k \"cd /d \"%s\" && venv\\Scripts\\python app.py\"", projectPath);
    runCommandVerbose(cmd, 1);
}



void deleteProject(const char *projectPath)
{
    char confirm[10];
    printf("Delete project '%s'? (y/n): ", projectPath);
    fgets(confirm, sizeof(confirm), stdin);
    if (confirm[0] == 'y' || confirm[0] == 'Y')
    {
        removeDirectoryCmd(projectPath);
        printf("Project deleted.\n");
    }
}



void addAsset(const char *projectPath)
{
    char type[10];
    printf("Add Directory or File? (d/f): ");
    fgets(type, sizeof(type), stdin);
    if (type[0] == 'd' || type[0] == 'D')
    {
        char dirName[BUFFER_SIZE];
        printf("Folder name: ");
        fgets(dirName, sizeof(dirName), stdin);
        dirName[strcspn(dirName, "\n")] = 0;
        char fullPath[BUFFER_SIZE];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", projectPath, dirName);
        createDirectoryCmd(fullPath);
        printf("Directory created.\n");
    }
    else if (type[0] == 'f' || type[0] == 'F')
    {
        char location[10];
        printf("At root level? (y/n): ");
        fgets(location, sizeof(location), stdin);
        char folder[BUFFER_SIZE] = "";
        if (location[0] != 'y' && location[0] != 'Y')
        {
            printf("Enter folder path relative to project root: ");
            fgets(folder, sizeof(folder), stdin);
            folder[strcspn(folder, "\n")] = 0;
        }
        char fileName[BUFFER_SIZE];
        printf("File name: ");
        fgets(fileName, sizeof(fileName), stdin);
        fileName[strcspn(fileName, "\n")] = 0;
        char fullPath[BUFFER_SIZE];
        if (strlen(folder) > 0)
            snprintf(fullPath, sizeof(fullPath), "%s\\%s\\%s", projectPath, folder, fileName);
        else
            snprintf(fullPath, sizeof(fullPath), "%s\\%s", projectPath, fileName);
        char cmd[CMD_BUFFER_SIZE];
        snprintf(cmd, sizeof(cmd), "echo. > \"%s\"", fullPath);
        runCommandVerbose(cmd, 0);
        printf("Blank file created.\n");
    }
    else
    {
        printf("Invalid choice.\n");
    }
}



void workspaceMenu(const char *projectPath)
{
    int choice;
    do
    {
        printf("\n--- Workspace Control ---\n");
        printf("Current project: %s\n", projectPath);
        printf("a) Install / Update dependencies (sync with requirements.txt)\n");
        printf("b) Run application (auto-syncs dependencies first)\n");
        printf("c) Delete project entirely\n");
        printf("d) Add a new workspace asset (file/directory)\n");
        printf("e) Exit to main menu\n");
        printf("Choice: ");
        char input[10];
        fgets(input, sizeof(input), stdin);
        choice = input[0];

        switch (choice)
        {
            case 'a':
            case 'A':
                installRequirements(projectPath);
                break;
            case 'b':
            case 'B':
                runFlaskApp(projectPath);
                break;
            case 'c':
            case 'C':
                deleteProject(projectPath);
                return;
            case 'd':
            case 'D':
                addAsset(projectPath);
                break;
            case 'e':
            case 'E':
                return;
            default:
                printf("Invalid choice.\n");
        }
    } while (1);
}



int main(void)
{
    char scopeChoice[10];
    int globalScope = 0;
    char *basePath = NULL;

    printf("=== Flask Project Manager (with .flaskman tracking) ===\n");
    printf("Manage projects for:\n");
    printf("1) Current user (%%USERPROFILE%%\\flask_manager)\n");
    printf("2) Global all users (%%SystemDrive%%\\Users\\flask_manager)\n");
    printf("Choice (1/2): ");
    fgets(scopeChoice, sizeof(scopeChoice), stdin);
    if (scopeChoice[0] == '2')
        globalScope = 1;

    basePath = getBasePath(globalScope);

    if (!directoryExists(basePath))
    {
        char answer[10];
        printf("Directory '%s' does not exist. Create it? (y/n): ", basePath);
        fgets(answer, sizeof(answer), stdin);
        if (answer[0] == 'y' || answer[0] == 'Y')
        {
            createDirectoryCmd(basePath);
        }
        else
        {
            printf("Cannot proceed without base directory. Exiting.\n");
            return 1;
        }
    }

    if (!checkPython())
    {
        installPythonChoice();
    }
    else
    {
        printf("Python is available.\n");
    }

    if (!checkVSCode())
    {
        installVSCode();
    }
    else
    {
        printf("Visual Studio Code is available.\n");
    }

    int mainChoice;
    char *currentProject = NULL;
    do
    {
        printf("\n--- Flask Project Manager Dashboard ---\n");
        printf("a) Spin a new project\n");
        printf("b) Open existing project\n");
        printf("c) Import a git repository\n");
        printf("d) Exit program\n");
        printf("Choice: ");
        char input[10];
        fgets(input, sizeof(input), stdin);
        mainChoice = input[0];

        switch (mainChoice)
        {
            case 'a':
            case 'A':
            {
                char projName[BUFFER_SIZE];
                printf("Project name: ");
                fgets(projName, sizeof(projName), stdin);
                projName[strcspn(projName, "\n")] = 0;
                char projectPath[BUFFER_SIZE];
                snprintf(projectPath, sizeof(projectPath), "%s\\%s", basePath, projName);
                if (directoryExists(projectPath))
                {
                    printf("Project already exists.\n");
                }
                else
                {
                    if (createFlaskProject(projectPath) == 0)
                    {
                        ensureVenv(projectPath);
                        currentProject = _strdup(projectPath);
                        workspaceMenu(currentProject);
                        free(currentProject);
                        currentProject = NULL;
                    }
                    else
                    {
                        printf("Failed to create project.\n");
                    }
                }
                break;
            }
            case 'b':
            case 'B':
            {
                char *sel = selectProject(basePath);
                if (sel)
                {
                    ensureVenv(sel);
                    currentProject = _strdup(sel);
                    workspaceMenu(currentProject);
                    free(currentProject);
                    currentProject = NULL;
                }
                break;
            }
            case 'c':
            case 'C':
            {
                char repoUrl[BUFFER_SIZE];
                char targetFolder[BUFFER_SIZE];
                printf("Git repository URL: ");
                fgets(repoUrl, sizeof(repoUrl), stdin);
                repoUrl[strcspn(repoUrl, "\n")] = 0;
                printf("Target workspace folder name: ");
                fgets(targetFolder, sizeof(targetFolder), stdin);
                targetFolder[strcspn(targetFolder, "\n")] = 0;
                char fullPath[BUFFER_SIZE];
                snprintf(fullPath, sizeof(fullPath), "%s\\%s", basePath, targetFolder);
                char cmd[CMD_BUFFER_SIZE];
                snprintf(cmd, sizeof(cmd), "git clone \"%s\" \"%s\"", repoUrl, fullPath);
                printf("Cloning repository...\n");
                runCommandVerbose(cmd, 1);
                printf("Clone completed.\n");
                break;
            }
            case 'd':
            case 'D':
                printf("Exiting.\n");
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (mainChoice != 'd' && mainChoice != 'D');

    return 0;
}
