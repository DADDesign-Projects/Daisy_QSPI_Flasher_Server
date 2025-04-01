//------------------------------------------------------------------------
// Copyright(c) 2024 Dad Design.
// 
// Utility to flash QSPIFlash memory from a PC
// Server management
//-----------------------------------------------------------------------

#include "pch.h"
#include <algorithm>
#include <windows.h>
#include <stdio.h>
#include <fstream>
#include "cServer.h"

// Server management class
// =======================

// Constructor to initialize the COM port handle and set the start and end markers
Dad::cServer::cServer() {
    m_hCom = INVALID_HANDLE_VALUE;
    memcpy_s(m_Bloc.StartMarker, sizeof(m_Bloc.StartMarker), "BLOC", 4);
    memcpy_s(m_Bloc.EndMarker, sizeof(m_Bloc.EndMarker), "END", 3);
}

// Destructor to close the COM port handle if it is open
Dad::cServer::~cServer() {
    ResetClass();
}

// Release the port and buffer memory
void Dad::cServer::ResetClass() {
    if (m_hCom != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hCom);
        m_hCom = INVALID_HANDLE_VALUE;
    }
    if (nullptr != m_pBuff) {
        delete[] m_pBuff;
        m_pBuff = nullptr;
        m_pEndBuff = nullptr;
        m_pFirstFreeBuff = nullptr;
        m_pFile = nullptr;
    }
}

// Configure the COM port with the specified parameters
bool Dad::cServer::Config(DWORD BaudRate, BYTE ByteSize, BYTE Parity, BYTE StopBits) {
    // Get the current COM port configuration
    if (!GetCommState(m_hCom, &m_Config)) {
        return false;
    }

    // Set the new COM port configuration
    m_Config.BaudRate = BaudRate;
    m_Config.ByteSize = ByteSize;
    m_Config.Parity = Parity;
    m_Config.StopBits = StopBits;

    // Apply the new configuration
    if (!SetCommState(m_hCom, &m_Config)) {
        return false;
    }

    // Confirm the configuration has been applied
    if (!GetCommState(m_hCom, &m_Config)) {
        return false;
    }

    // Clear the COM port input buffer
    PurgeComm(m_hCom, PURGE_RXCLEAR);
    return true;
}

// Initialize the COM port with default or specified parameters
bool Dad::cServer::Init(uint8_t NumPort, uint32_t QSPi_Size, DWORD BaudRate, BYTE ByteSize, BYTE Parity, BYTE StopBits) {
    ResetClass();

    // Initialize the buffer
    if (QSPi_Size > QSPI_SIZE) {
        return false;
    }
    m_QSPI_Size = QSPi_Size;
    m_pBuff = new uint8_t[QSPi_Size];
    if (nullptr == m_pBuff) {
        return false;
    }
    memset(m_pBuff, 0, QSPi_Size);
    m_pFile = (stFile*)m_pBuff;
    m_pEndBuff = m_pBuff + QSPi_Size;
    m_pFirstFreeBuff = m_pBuff + sizeof(Directory);

    wchar_t NomPort[10];
    swprintf_s(NomPort, 10, L"\\\\.\\COM%d", NumPort);

    // Open the COM port
    m_hCom = CreateFile(NomPort,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (m_hCom == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Initialize the DCB (Device Control Block) structure
    SecureZeroMemory(&m_Config, sizeof(DCB));
    m_Config.DCBlength = sizeof(DCB);

    // Configure the COM port with the specified parameters
    if (!Config(BaudRate, ByteSize, Parity, StopBits)) {
        CloseHandle(m_hCom);
        m_hCom = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };

    // Set the timeouts for non-blocking reads
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(m_hCom, &timeouts)) {
        CloseHandle(m_hCom);
        m_hCom = INVALID_HANDLE_VALUE;
        return false;
    }
    return true;
}

// Synchronize with the COM port and retrieve the block number
int16_t Dad::cServer::Synchronize() {
    DWORD NbCharLus = 0;
    uint8_t Buff;
    int16_t NumBloc;

    // Define the synchronization steps
    enum class eStep {
        Marker_B,
        Marker_L,
        Marker_O,
        Marker_C,
        NumBloc_1,
        NumBloc_2
    } Step = eStep::Marker_B;

    // Get the start time in milliseconds
    uint64_t startTime = GetTickCount64();
    uint64_t timeout = 5000; // 5 seconds

    while (GetTickCount64() - startTime < timeout) {
        // Read one byte from the COM port
        if (!ReadFile(m_hCom, &Buff, 1, &NbCharLus, NULL)) {
            return -1;
        }
        if (1 == NbCharLus) {
            // Perform the synchronization step
            switch (Step) {
            case eStep::Marker_B:
                if (Buff == 'B') {
                    Step = eStep::Marker_L;
                }
                break;
            case eStep::Marker_L:
                if (Buff == 'L') {
                    Step = eStep::Marker_O;
                }
                else {
                    Step = eStep::Marker_B;
                }
                break;
            case eStep::Marker_O:
                if (Buff == 'O') {
                    Step = eStep::Marker_C;
                }
                else {
                    Step = eStep::Marker_B;
                }
                break;
            case eStep::Marker_C:
                if (Buff == 'C') {
                    Step = eStep::NumBloc_1;
                }
                else {
                    Step = eStep::Marker_B;
                }
                break;
            case eStep::NumBloc_1:
                NumBloc = Buff;
                Step = eStep::NumBloc_2;
                break;
            case eStep::NumBloc_2:
                NumBloc += (Buff << 8);
                return NumBloc;
            }
        }
    }
    return -1;
}

// Transmit a block of data via the COM port
bool Dad::cServer::TransBloc(uint16_t NumBloc, uint8_t EndTrans) {
    // Clear the COM port input buffer
    PurgeComm(m_hCom, PURGE_RXCLEAR);
    uint8_t* pData = m_pBuff + (NumBloc * TRANS_BLOCK_SIZE);
    // Copy the data block and calculate the CRC
    uint8_t* pBlocData = m_Bloc.Data;
    uint8_t CalcCRC = 0;
    for (uint16_t Index = 0; Index < TRANS_BLOCK_SIZE; Index++) {
        CalcCRC += *pData;
        *pBlocData++ = *pData++;
    }

    // Set the other fields of the block
    m_Bloc.NumBloc = NumBloc;
    m_Bloc._EndTrans = EndTrans;
    m_Bloc._CRC = CalcCRC;
    DWORD NbCharWrite = 0;

    // Write the block to the COM port
    if (!WriteFile(m_hCom, &m_Bloc, sizeof(Dad::Bloc), &NbCharWrite, NULL) ||
        NbCharWrite != sizeof(Dad::Bloc)) {
        return false;
    }
    return true;
}

// Add a file to the transfer buffer
bool Dad::cServer::addFile(const std::string& filePath, const std::string& fileName) {
    if (m_IndexFile >= DIR_FILE_COUNT) {
        return false;
    }
    if (true == isImageFile(fileName)) {
        return addImageFile(filePath, fileName);
    }else {
        return addCommonFile(filePath, fileName);
    }
}
// Add a image file to the transfer buffer
bool Dad::cServer::addImageFile(const std::string& filePath, const std::string& fileName) {
    CImage image;
    HBITMAP hBmp;

	// Load image file
	if (FAILED(image.Load(CString(filePath.c_str())))) {
		return false;
	}

	// Get image dimensions
	int width = image.GetWidth();
	int height = image.GetHeight();
	int bpp = image.GetBPP();

    // Check for free space in buffer
	if ((m_pFirstFreeBuff + width * height * bpp / 8) > m_pEndBuff) {
		return false;
	}
    
    // Create file entry in directory
    strncpy_s(m_pFile->Name, fileName.c_str(), MAX_ENTRY_NAME - 1);
    m_pFile->Size = width * height * bpp / 8;
    m_pFile->DataAddress = QSPI_ADRESSE + (m_pFirstFreeBuff - m_pBuff);

	// Copy image data to buffer
	if (bpp == 24) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				COLORREF color = image.GetPixel(x, y);
				*m_pFirstFreeBuff++ = GetBValue(color);
				*m_pFirstFreeBuff++ = GetGValue(color);
				*m_pFirstFreeBuff++ = GetRValue(color);
                *m_pFirstFreeBuff++ = 255;
			}
		}
	}
    else if (bpp == 32) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint8_t *pPixel = (uint8_t *) image.GetPixelAddress(x,y);
                *m_pFirstFreeBuff++ = pPixel[2];
                *m_pFirstFreeBuff++ = pPixel[1];
                *m_pFirstFreeBuff++ = pPixel[0];
                *m_pFirstFreeBuff++ = pPixel[3];
            }
        }
    }
	else {
		return false;
	}

    // Increment pointers
	m_pFile++;
	return true;
}

// Add a common file to the transfer buffer
bool Dad::cServer::addCommonFile(const std::string& filePath, const std::string& fileName) {

    // Open the file
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    // Get file size
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Check for free space in buffer
    if ((m_pFirstFreeBuff + fileSize) > m_pEndBuff) {
        file.close();
        return false;
    }

    // Copy file data to buffer
    if (!file.read((char*)m_pFirstFreeBuff, fileSize)) {
        file.close();
        return false;
    }

    // Create file entry in directory
    strncpy_s(m_pFile->Name, fileName.c_str(), MAX_ENTRY_NAME - 1);
    m_pFile->Size = fileSize;
    m_pFile->DataAddress = QSPI_ADRESSE + (m_pFirstFreeBuff - m_pBuff);

    // Increment pointers
    m_pFirstFreeBuff += fileSize;
    // Align to 4 bytes
    m_pFirstFreeBuff = (uint8_t*)(((uintptr_t)m_pFirstFreeBuff + 3) & ~3);
    
    m_pFile++;

    file.close();
    return true;
}
// Test if file is image file
bool  Dad::cServer::isImageFile(const std::string& FileName) {
    // List of valid extensions
    const std::string validExtensions[] = { ".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff", ".gif" };

    // Find the position of the last dot (.)
    size_t dotPosition = FileName.find_last_of('.');
    if (dotPosition == std::string::npos) {
        return false; // No extension found
    }

    // Extract the file extension and convert it to lowercase
    std::string extension = FileName.substr(dotPosition);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Check if the extension matches any in the list of valid extensions
    for (const auto& validExtension : validExtensions) {
        if (extension == validExtension) {
            return true; // Valid image file
        }
    }

    return false; // Not a valid image file
}
