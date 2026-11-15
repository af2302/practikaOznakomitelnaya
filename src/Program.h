#pragma once
#include <QString>
#include <cstddef>
#include <deque>
#include <vector>

namespace music {

inline constexpr int kMaxTracks = 1000;
inline constexpr int kMaxCode = 1'000'000;
inline constexpr int kMaxText = 100;
inline constexpr int kMinDuration = 1;
inline constexpr int kMaxDuration = 86'400;

struct Track {
    int code = 0;
    QString title;
    QString artist;
    int duration = 0;
};

// Результат операции: ok == true либо понятное описание ошибки.
struct Result {
    bool ok = true;
    QString error;
    explicit operator bool() const { return ok; }
    static Result success() { return {}; }
    static Result fail(QString message) { return Result{false, std::move(message)}; }
};

// Предметная логика. Не зависит от виджетов.
// Все операции при отказе оставляют состояние неизменным.
class Program {
public:
    Result addFront(Track track);
    Result addBack(Track track);
    Result removeFront();
    Result removeBack();
    Result rotateFirst(int k);
    Result setDuration(int code, int seconds);

    // Поиск по фрагменту названия без учёта регистра; пустой фрагмент даёт всё.
    std::vector<Track> search(const QString& fragment) const;
    long long totalDuration() const;
    const Track* find(int code) const;

    std::vector<int> codesForward() const;
    std::vector<int> codesBackward() const;

    std::size_t size() const { return m_tracks.size(); }
    bool empty() const { return m_tracks.empty(); }
    const std::deque<Track>& tracks() const { return m_tracks; }

    // Проверка одного номера (после обрезки пробелов) и всей коллекции.
    static Result validateTrack(const Track& track);
    static Result validateAll(const std::deque<Track>& tracks);
    // Заменяет всё состояние целиком после успешной проверки.
    Result replaceAll(std::deque<Track> tracks);

private:
    Result prepare(Track& track) const;
    std::deque<Track> m_tracks;
};

}  // namespace music
