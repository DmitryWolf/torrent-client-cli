#pragma once

#include "torrent_file.h"
#include "piece.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>

/*
 * Хранилище информации о частях скачиваемого файла.
 * В этом классе отслеживается информация о том, какие части файла осталось скачать
 */

namespace fs = std::filesystem;

class PieceStorage {
public:
    PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, const std::string& outputTempFileName);

    ~PieceStorage();

    /*
     * Отдает указатель на следующую часть файла, которую надо скачать
     */
    PiecePtr GetNextPieceToDownload();

    /*
     * Эта функция вызывается из PeerConnect, когда скачивание одной части файла завершено.
     * В рамках данного задания требуется очистить очередь частей для скачивания как только хотя бы одна часть будет успешно скачана.
     */
    void PieceProcessed(const PiecePtr& piece);

    /*
     * Остались ли нескачанные части файла?
     */
    bool QueueIsEmpty() const;

    /*
     * Сколько частей файла было сохранено на диск
     */
    size_t PiecesSavedToDiscCount() const;

    /*
     * Сколько частей файла всего
     */
    size_t TotalPiecesCount() const;

    /*
     * Закрыть поток вывода в файл
     */
    void CloseOutputFile();

    /*
     * Отдает список номеров частей файла, которые были сохранены на диск
     */
    const std::vector<size_t>& GetPiecesSavedToDiscIndices() const;

    /*
     * Сколько частей файла в данный момент скачивается
     */
    size_t PiecesInProgressCount() const;

    /*
     * Метод создан для того, чтобы при возникновении ошибки или вызова Terminate из другого потока 
     * мы смогли нормализовать счетчик частей файла, которые скачиваются в данный момент
     */
    void DecrementPieceInProgressCounter();

    /*
     * Если при скачивании блоков части возникла ошибка, то мы должны вернуть часть в очередь
     */
    void BackPieceToQueue(const size_t pieceIndex);

    /*
     * Установить максимальное количество частей файла (первые N частей)
     */
    void SetNewSize(const size_t newSize);

private:
    std::deque<PiecePtr> remainPieces_; // очередь частей файла
    std::unordered_map<size_t, PiecePtr> downloadingPieces_; // хеш-мапа с частями файла, которые скачиваются в данный момент. Ключ - индекс, значение - PiecePtr
    
    size_t totalSize_; // общее количество частей файла
    mutable std::mutex mutex_; // mutex для критических секций
    std::vector<size_t> indicesOfSavedPiecesToDisc_; // индексы сохранненных на диск частей файла
    std::string fileName_; // название скачиваемого файла
    std::atomic<size_t> piecesInProgressCount_; // количество частей файла, скачивающихся в данный момент
    int fd_; // filedescriptor для временного файла
    size_t pieceLength_; // длина части (данные из .torrent, размер последней части может отличаться)
    /*
     * Сохраняет данную скачанную часть файла на диск.
     * Сохранение всех частей происходит в один выходной файл. Позиция записываемых данных зависит от индекса части
     * и размера частей. Данные, содержащиеся в части файла, должны быть записаны сразу в правильную позицию.
     */
    void SavePieceToDisk(const PiecePtr& piece);
    int OpenFile();
    void CloseFile();
};

