// src/server/transaction/WALManager.cpp
#include "WALManager.h"
#include "../../common/Logger.h"
#include <sys/stat.h>
#include <algorithm>

namespace minisql {

WALManager::~WALManager() {
    if (walFile_.is_open()) {
        walFile_.flush();
        walFile_.close();
    }
}

WALManager& WALManager::getInstance() {
    static WALManager instance;
    return instance;
}

void WALManager::init(const std::string& dataDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    walPath_ = dataDir + "/wal/minisql.wal";

    walFile_.open(walPath_, std::ios::app | std::ios::binary);
    if (!walFile_.is_open()) {
        LOG_INFO("WALManager", "Creating WAL directory");
    }
}

uint64_t WALManager::getNextLSN() {
    return ++currentLSN_;
}

void WALManager::writeLog(uint64_t txnId, LogType type,
                          const std::string& table,
                          const std::string& key,
                          const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t lsn = getNextLSN();
    uint8_t typeByte = static_cast<uint8_t>(type);

    // 序列化: LSN(8) + TxnID(8) + Type(1) + TableLen(4) + KeyLen(4) + ValueLen(4) + Data
    uint32_t tableLen = static_cast<uint32_t>(table.size());
    uint32_t keyLen = static_cast<uint32_t>(key.size());
    uint32_t valueLen = static_cast<uint32_t>(value.size());

    walFile_.write(reinterpret_cast<const char*>(&lsn), sizeof(lsn));
    walFile_.write(reinterpret_cast<const char*>(&txnId), sizeof(txnId));
    walFile_.write(reinterpret_cast<const char*>(&typeByte), sizeof(typeByte));
    walFile_.write(reinterpret_cast<const char*>(&tableLen), sizeof(tableLen));
    walFile_.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
    walFile_.write(reinterpret_cast<const char*>(&valueLen), sizeof(valueLen));
    walFile_.write(table.c_str(), table.size());
    walFile_.write(key.c_str(), key.size());
    walFile_.write(value.c_str(), value.size());

    walFile_.flush();  // 确保写入磁盘
}

void WALManager::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (walFile_.is_open()) {
        walFile_.flush();
    }
}

std::vector<WALEntry> WALManager::recover() {
    std::vector<WALEntry> entries;
    std::ifstream walFile(walPath_, std::ios::binary);

    if (!walFile.is_open()) {
        LOG_INFO("WALManager", "No WAL file found, skipping recovery");
        return entries;
    }

    while (walFile.peek() != EOF) {
        uint64_t lsn, txnId;
        uint8_t typeByte;
        uint32_t tableLen, keyLen, valueLen;

        walFile.read(reinterpret_cast<char*>(&lsn), sizeof(lsn));
        walFile.read(reinterpret_cast<char*>(&txnId), sizeof(txnId));
        walFile.read(reinterpret_cast<char*>(&typeByte), sizeof(typeByte));
        walFile.read(reinterpret_cast<char*>(&tableLen), sizeof(tableLen));
        walFile.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
        walFile.read(reinterpret_cast<char*>(&valueLen), sizeof(valueLen));

        std::string table(tableLen, '\0');
        std::string key(keyLen, '\0');
        std::string value(valueLen, '\0');

        if (tableLen > 0) walFile.read(&table[0], tableLen);
        if (keyLen > 0) walFile.read(&key[0], keyLen);
        if (valueLen > 0) walFile.read(&value[0], valueLen);

        entries.push_back({txnId, static_cast<LogType>(typeByte), table, key, value, lsn});
    }

    LOG_INFO("WALManager", "Recovered " + std::to_string(entries.size()) + " WAL entries");
    return entries;
}

void WALManager::truncate() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (walFile_.is_open()) {
        walFile_.close();
    }
    std::remove(walPath_.c_str());
    walFile_.open(walPath_, std::ios::app | std::ios::binary);
    currentLSN_ = 0;
}

}  // namespace minisql
