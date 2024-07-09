//------------------------------------------------------------------------
// Copyright(c) 2024 Dad Design.
// 
// Utility to flash QSPIFlash memory from a PC
// Server management
//-----------------------------------------------------------------------
#pragma once
#include <string>
#include <stdint.h>
#include "Flasher.h"

namespace Dad {
    // Server management class
    class cServer {
    public:
        // Constructor to initialize the COM port handle and set the start and end markers
        cServer();

        // Destructor to close the COM port handle if it is open
        ~cServer();

        // Initialize the COM port with default or specified parameters
        bool Init(uint8_t NumPort, uint32_t QSPi_Size = QSPI_PAGE_SIZE * 2, DWORD BaudRate = CBR_9600, BYTE ByteSize = 8, BYTE Parity = NOPARITY, BYTE StopBits = ONESTOPBIT);

        // Synchronize with the COM port and retrieve the block number
        int16_t Synchronize();

        // Transmit a block of data via the COM port
        bool TransBloc(uint16_t NumBloc, uint8_t EndTrans = 0);

        // Add a file to the transfer buffer
        bool addFile(const std::string& filePath, const std::string& fileName);

        // Read the size of loaded data
        uint32_t getDataSize() {
            return m_pFirstFreeBuff - m_pBuff;
        }

        // Read the number of blocks to transfer
        uint16_t getNbBlocs() {
            uint32_t NbBlocs1 = (m_pFirstFreeBuff - m_pBuff);
            uint16_t NbBlocs = (NbBlocs1 / TRANS_BLOCK_SIZE) + 1;
            return (NbBlocs);
        }

    protected:
        // Release the port and buffer memory
        void ResetClass();

        // Configure the COM port with the specified parameters
        bool Config(DWORD BaudRate, BYTE ByteSize, BYTE Parity, BYTE StopBits);

        // QSPI buffer
        uint32_t    m_QSPI_Size = 0;                // QSPI buffer size
        uint8_t*    m_pBuff = nullptr;              // Pointer to the start of the QSPI buffer
        uint8_t*    m_pEndBuff = nullptr;           // Pointer to the end of the QSPI buffer
        uint8_t*    m_pFirstFreeBuff = nullptr;     // Pointer to the first free byte in the QSPI buffer

        stFile*     m_pFile = nullptr;              // Pointer to the current file structure in the buffer
        uint8_t     m_IndexFile = 0;                // Index of the current file in the directory

        // COM port
        HANDLE      m_hCom;         // Handle for the COM port
        DCB         m_Config;       // Device Control Block structure
        Dad::Bloc   m_Bloc;         // Data block structure
    };
}
