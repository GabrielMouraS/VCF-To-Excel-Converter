#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h> // Para SHGetFolderPath e CSIDL_DESKTOP

#define ID_BUTTON_OPEN 1
#define ID_BUTTON_CONVERT 2
#define ID_CHECKBOX_CSV 3
#define ID_CHECKBOX_TSV 4

HWND hEditInput, hEditOutput, hButtonOpen, hButtonConvert;
HWND hCheckboxCSV, hCheckboxTSV;

typedef struct {
    char name[100];
    char phone[20];
} Contact;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
Contact* read_vcf(const char* filename, int* count);
void save_to_csv(const char* filename, Contact* contacts, int count);
void save_to_tsv(const char* filename, Contact* contacts, int count);
void save_file(Contact* contacts, int count, const char* format);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "VCFConverterClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, "VCFConverterClass", "VCF Converter", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hEditInput = CreateWindow("EDIT", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                      10, 10, 300, 20, hwnd, NULL, NULL, NULL);
            hButtonOpen = CreateWindow("BUTTON", "Abrir VCF", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                       10, 40, 100, 30, hwnd, (HMENU)ID_BUTTON_OPEN, NULL, NULL);
            hEditOutput = CreateWindow("EDIT", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                       10, 80, 300, 20, hwnd, NULL, NULL, NULL);
            hCheckboxCSV = CreateWindow("BUTTON", "CSV", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                         10, 120, 100, 20, hwnd, (HMENU)ID_CHECKBOX_CSV, NULL, NULL);
            hCheckboxTSV = CreateWindow("BUTTON", "TSV", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                         120, 120, 100, 20, hwnd, (HMENU)ID_CHECKBOX_TSV, NULL, NULL);
            hButtonConvert = CreateWindow("BUTTON", "Converter", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                          10, 180, 100, 30, hwnd, (HMENU)ID_BUTTON_CONVERT, NULL, NULL);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BUTTON_OPEN) {
                OPENFILENAME ofn;
                char file[260] = {0};
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = file;
                ofn.nMaxFile = sizeof(file);
                ofn.lpstrFilter = "VCF Files (*.vcf)\0*.vcf\0All Files (*.*)\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                if (GetOpenFileName(&ofn)) {
                    SetWindowText(hEditInput, file);
                }
            } else if (LOWORD(wParam) == ID_BUTTON_CONVERT) {
                char inputFile[260], outputFile[260];
                GetWindowText(hEditInput, inputFile, 260);

                int count;
                Contact* contacts = read_vcf(inputFile, &count);

                if (contacts) {
                    char format[10] = "csv";
                    if (SendMessage(hCheckboxCSV, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        strcpy(format, "csv");
                    } else if (SendMessage(hCheckboxTSV, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        strcpy(format, "tsv");
                    }

                    save_file(contacts, count, format);
                    free(contacts);
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

Contact* read_vcf(const char* filename, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        MessageBox(NULL, "Erro ao abrir o arquivo VCF.", "Erro", MB_ICONERROR);
        return NULL;
    }

    Contact* contacts = (Contact*)malloc(1000 * sizeof(Contact));
    char line[256];
    Contact currentContact = {0};
    *count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "BEGIN:VCARD")) {
            memset(&currentContact, 0, sizeof(Contact));
        } else if (strstr(line, "FN:")) {
            sscanf(line, "FN:%[^\n]", currentContact.name);
        } else if (strstr(line, "TEL;CELL:")) {
            sscanf(line, "TEL;CELL:%[^\n]", currentContact.phone);
        } else if (strstr(line, "END:VCARD")) {
            if (strlen(currentContact.name) > 0 && strlen(currentContact.phone) > 0) {
                contacts[(*count)++] = currentContact;
                if (*count >= 1000) {
                    contacts = (Contact*)realloc(contacts, (*count + 1000) * sizeof(Contact));
                }
            }
        }
    }

    fclose(file);
    return contacts;
}

void save_to_csv(const char* filename, Contact* contacts, int count) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        MessageBox(NULL, "Erro ao criar o arquivo CSV.", "Erro", MB_ICONERROR);
        return;
    }

    fprintf(file, "Nome,Telefone\n");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s,%s\n", contacts[i].name, contacts[i].phone);
    }

    fclose(file);
}

void save_to_tsv(const char* filename, Contact* contacts, int count) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        MessageBox(NULL, "Erro ao criar o arquivo TSV.", "Erro", MB_ICONERROR);
        return;
    }

    fprintf(file, "Nome\tTelefone\n");
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s\t%s\n", contacts[i].name, contacts[i].phone);
    }

    fclose(file);
}

void save_file(Contact* contacts, int count, const char* format) {
    char path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path);

    char filename[MAX_PATH];
    sprintf(filename, "%s\\contatos.%s", path, format);

    if (strcmp(format, "csv") == 0) {
        save_to_csv(filename, contacts, count);
    } else if (strcmp(format, "tsv") == 0) {
        save_to_tsv(filename, contacts, count);
    }

    char message[100];
    sprintf(message, "Conversão realizada com sucesso!\nContatos convertidos: %d", count);
    MessageBox(NULL, message, "Sucesso", MB_ICONINFORMATION);
}