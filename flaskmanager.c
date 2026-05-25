#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096
#define CMD_BUFFER_SIZE 8192



int runCommand(const char *cmdLine)
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    char cmd[CMD_BUFFER_SIZE];

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    snprintf(cmd, sizeof(cmd), "cmd.exe /c \"%s\"", cmdLine);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return -1;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;
}



int directoryExists(const char *path)
{
    DWORD attrs = GetFileAttributesA(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}



int createDirectoryCmd(const char *path)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\"", path);
    return runCommand(cmd);
}



int removeDirectoryCmd(const char *path)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\"", path);
    return runCommand(cmd);
}



int writeFileContent(const char *path, const char *content)
{
    char cmd[CMD_BUFFER_SIZE];
    char escaped[CMD_BUFFER_SIZE];
    const char *src = content;
    char *dst = escaped;
    while (*src && (dst - escaped) < (CMD_BUFFER_SIZE - 10))
    {
        if (*src == '>' || *src == '<' || *src == '|' || *src == '&' || *src == '%')
        {
            *dst++ = '^';
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    snprintf(cmd, sizeof(cmd), "echo %s > \"%s\"", escaped, path);
    return runCommand(cmd);
}



int appendFileContent(const char *path, const char *content)
{
    char cmd[CMD_BUFFER_SIZE];
    char escaped[CMD_BUFFER_SIZE];
    const char *src = content;
    char *dst = escaped;
    while (*src && (dst - escaped) < (CMD_BUFFER_SIZE - 10))
    {
        if (*src == '>' || *src == '<' || *src == '|' || *src == '&' || *src == '%')
        {
            *dst++ = '^';
        }
        *dst++ = *src++;
    }
    *dst = '\0';
    snprintf(cmd, sizeof(cmd), "echo %s >> \"%s\"", escaped, path);
    return runCommand(cmd);
}



int checkPython(void)
{
    int ret = runCommand("python --version > nul 2>&1");
    return (ret == 0);
}



int checkVSCode(void)
{
    int ret = runCommand("code --version > nul 2>&1");
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
            runCommand("winget install --id Python.Python --silent --accept-package-agreements");
            break;
        }
        else if (input[0] == 'b' || input[0] == 'B')
        {
            runCommand("winget install --id Python.Python.3.12 --silent --accept-package-agreements");
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
    runCommand("winget install --id Microsoft.VisualStudioCode --silent --accept-package-agreements");
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



void createFlaskProject(const char *projectPath, const char *projectName)
{
    char cmd[CMD_BUFFER_SIZE];
    char templatesPath[BUFFER_SIZE];
    char staticPath[BUFFER_SIZE];
    char appPath[BUFFER_SIZE];
    char indexPath[BUFFER_SIZE];

    snprintf(templatesPath, sizeof(templatesPath), "%s\\templates", projectPath);
    snprintf(staticPath, sizeof(staticPath), "%s\\static", projectPath);
    snprintf(appPath, sizeof(appPath), "%s\\app.py", projectPath);
    snprintf(indexPath, sizeof(indexPath), "%s\\templates\\index.html", projectPath);

    createDirectoryCmd(projectPath);
    createDirectoryCmd(templatesPath);
    createDirectoryCmd(staticPath);

    writeFileContent(appPath, "from flask import Flask, render_template");
    appendFileContent(appPath, "");
    appendFileContent(appPath, "app = Flask(__name__)");
    appendFileContent(appPath, "");
    appendFileContent(appPath, "@app.route('/')");
    appendFileContent(appPath, "def home():");
    appendFileContent(appPath, "    return render_template('index.html')");
    appendFileContent(appPath, "");
    appendFileContent(appPath, "if __name__ == '__main__':");
    appendFileContent(appPath, "    app.run(debug=True)");

    writeFileContent(indexPath, "<!DOCTYPE html>");
    appendFileContent(indexPath, "<html>");
    appendFileContent(indexPath, "<head><title>Flask App</title></head>");
    appendFileContent(indexPath, "<body>");
    appendFileContent(indexPath, "<h1>Hello from Flask!</h1>");
    appendFileContent(indexPath, "</body>");
    appendFileContent(indexPath, "</html>");
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
        runCommand(pythonCmd);
    }
}



void installFlaskVenv(const char *projectPath)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "cd /d \"%s\" && venv\\Scripts\\pip install flask", projectPath);
    runCommand(cmd);
    printf("Flask installed.\n");
}



void runFlaskApp(const char *projectPath)
{
    char cmd[CMD_BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "start cmd.exe /k \"cd /d \"%s\" && venv\\Scripts\\python app.py\"", projectPath);
    runCommand(cmd);
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
        runCommand(cmd);
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
        printf("a) Install dependencies (flask)\n");
        printf("b) Run application (python app.py)\n");
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
                installFlaskVenv(projectPath);
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
    printf("Welcome to flaskmanager V1.0 by Tole CaxtoneKirigha! Feel free to contribute to this project at: https://github.com/ToleInventor/flaskmanager\n");
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

    if (!checkVSCode())
    {
        installVSCode();
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
                    createFlaskProject(projectPath, projName);
                    ensureVenv(projectPath);
                    currentProject = _strdup(projectPath);
                    workspaceMenu(currentProject);
                    free(currentProject);
                    currentProject = NULL;
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
                runCommand(cmd);
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
