#define WIN32_LEAN_AND_MEAN

#include "injector.h"
#include "ui_injector.h"
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

#include <string.h>
#include <windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <wchar.h>
#include <iostream>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <thread>
#include <Scripts.h>

// Libraries to be linked into the executable.
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// The name of our injector DLL.
const char* DLL_NAME = "C:\\Program Files (x86)\\Disney\\Disney Online\\ToontownOnline\\pyloader.dll";

// The buffer length and port for our socket connection.
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "1337"

void PrintLastErrorMsg() {
   LPTSTR pTmp = NULL;
   DWORD errnum = GetLastError();
   FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL, 
                errnum, 
                LANG_NEUTRAL,
                (LPTSTR)&pTmp, 
                0,
                NULL
                );

   std::cout << "Error(" << errnum << "): " << pTmp << std::endl;
}

Injector::Injector(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Injector)
{
    ui->setupUi(this);

    // Setup our runtime handle.
    HANDLE runtime = this->setupInjector();
}

HANDLE Injector::setupInjector() {
  // Lets check if the handle is valid.
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  HANDLE handleToProc = nullptr;

  if (hSnapshot) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

      if (Process32First(hSnapshot, &pe32)) {
        do {
          if(!wcscmp(pe32.szExeFile, L"Toontown.exe")) {
            std::wcout << "processID: " << pe32.th32ProcessID;

            handleToProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pe32.th32ProcessID);
            LPVOID MemAlloc = VirtualAllocEx(handleToProc, NULL, strlen(DLL_NAME) + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
            int retVal = WriteProcessMemory(handleToProc, MemAlloc, DLL_NAME, strlen(DLL_NAME) + 1, NULL);
            printf("\nretVal - %d\n", retVal);

            if (retVal == 0) {
              // HOW
              PrintLastErrorMsg();
            }
  
            LPVOID LoadLibAddress = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

            CreateRemoteThread(handleToProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibAddress, MemAlloc, 0, NULL);
            }
        } while (Process32Next(hSnapshot, &pe32));
      }
  }
    // Return our process handle.
    return handleToProc;
}

Injector::~Injector()
{
    delete ui;
}

int Injector::on_PhaseCheck_triggered()
{
    // While this works, ideally we'd be calling PyRun_SimpleStringFlags instead.
    int retVal = this->sendPythonPayload(skipPhaseChk);
    return retVal;
}

int Injector::sendPythonPayload(const char* text) {

    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = ::connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send our Python payload.
    iResult = send( ConnectSocket, text, (int)strlen(text), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("Failed to send payload with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // Shutdown the connection since no more data will be sent.
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("Failed to shutdown with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Cleanup.
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

int Injector::on_InjectCode_clicked()
{
    QString text = this->ui->CodeInput->toPlainText();
    std::string str = text.toStdString();
    const char* p = str.c_str();

    // While this works, ideally we'd be calling PyRun_SimpleStringFlags instead.
    int retVal = this->sendPythonPayload(p);
    return retVal;
}

void Injector::on_Clear_clicked()
{
    this->ui->CodeInput->clear();
}

void Injector::on_actionTeam_Legend_triggered()
{
    // While this works, ideally we'd be calling PyRun_SimpleStringFlags instead.
    this->sendPythonPayload(teamLegendButtons);
}

void Injector::on_actionBytecode_Extract_triggered()
{
    // While this works, ideally we'd be calling PyRun_SimpleStringFlags instead.
    this->sendPythonPayload(extractBytecode);
}

