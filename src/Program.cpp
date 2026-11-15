#include "Program.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <set>

namespace music {

Result Program::validateTrack(const Track& t) {
    if (t.code < 1 || t.code > kMaxCode)
        return Result::fail(QStringLiteral("Код должен быть целым числом от 1 до 1000000."));
    const QString title = t.title.trimmed();
    const QString artist = t.artist.trimmed();
    if (title.isEmpty() || title.size() > kMaxText)
        return Result::fail(QStringLiteral("Название должно содержать от 1 до 100 символов."));
    if (artist.isEmpty() || artist.size() > kMaxText)
        return Result::fail(QStringLiteral("Исполнитель должен содержать от 1 до 100 символов."));
    if (t.duration < kMinDuration || t.duration > kMaxDuration)
        return Result::fail(QStringLiteral("Продолжительность должна быть от 1 до 86400 секунд."));
    return Result::success();
}

Result Program::validateAll(const std::deque<Track>& tracks) {
    if (tracks.size() > static_cast<std::size_t>(kMaxTracks))
        return Result::fail(QStringLiteral("Превышен предел в 1000 номеров."));
    std::set<int> seen;
    for (const Track& t : tracks) {
        if (Result r = validateTrack(t); !r)
            return r;
        if (!seen.insert(t.code).second)
            return Result::fail(QStringLiteral("Повторяющийся код номера: %1.").arg(t.code));
    }
    return Result::success();
}

Result Program::prepare(Track& t) const {
    t.title = t.title.trimmed();
    t.artist = t.artist.trimmed();
    if (Result r = validateTrack(t); !r)
        return r;
    if (m_tracks.size() >= static_cast<std::size_t>(kMaxTracks))
        return Result::fail(QStringLiteral("Превышен предел в 1000 номеров."));
    if (find(t.code) != nullptr)
        return Result::fail(QStringLiteral("Номер с кодом %1 уже есть в программе.").arg(t.code));
    return Result::success();
}

Result Program::addFront(Track t) {
    if (Result r = prepare(t); !r)
        return r;
    m_tracks.push_front(std::move(t));
    return Result::success();
}

Result Program::addBack(Track t) {
    if (Result r = prepare(t); !r)
        return r;
    m_tracks.push_back(std::move(t));
    return Result::success();
}

Result Program::removeFront() {
    if (m_tracks.empty())
        return Result::fail(QStringLiteral("Программа пуста: отменять нечего."));
    m_tracks.pop_front();
    return Result::success();
}

Result Program::removeBack() {
    if (m_tracks.empty())
        return Result::fail(QStringLiteral("Программа пуста: отменять нечего."));
    m_tracks.pop_back();
    return Result::success();
}

Result Program::rotateFirst(int k) {
    if (k < 0 || static_cast<std::size_t>(k) > m_tracks.size())
        return Result::fail(
            QStringLiteral("k должно быть от 0 до %1 включительно.").arg(m_tracks.size()));
    // Итераторы получаем заново, после операции они не используются.
    std::rotate(m_tracks.begin(), m_tracks.begin() + k, m_tracks.end());
    return Result::success();
}

Result Program::setDuration(int code, int seconds) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(),
                           [code](const Track& t) { return t.code == code; });
    if (it == m_tracks.end())
        return Result::fail(QStringLiteral("Номер с кодом %1 не найден.").arg(code));
    if (seconds < kMinDuration || seconds > kMaxDuration)
        return Result::fail(QStringLiteral("Продолжительность должна быть от 1 до 86400 секунд."));
    it->duration = seconds;
    return Result::success();
}

std::vector<Track> Program::search(const QString& fragment) const {
    const QString f = fragment.trimmed();
    std::vector<Track> out;
    std::copy_if(m_tracks.begin(), m_tracks.end(), std::back_inserter(out),
                 [&f](const Track& t) {
                     return f.isEmpty() || t.title.contains(f, Qt::CaseInsensitive);
                 });
    return out;
}

long long Program::totalDuration() const {
    return std::accumulate(m_tracks.begin(), m_tracks.end(), 0LL,
                           [](long long sum, const Track& t) { return sum + t.duration; });
}

const Track* Program::find(int code) const {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(),
                           [code](const Track& t) { return t.code == code; });
    return it == m_tracks.end() ? nullptr : &*it;
}

std::vector<int> Program::codesForward() const {
    std::vector<int> out;
    for (auto it = m_tracks.cbegin(); it != m_tracks.cend(); ++it)
        out.push_back(it->code);
    return out;
}

std::vector<int> Program::codesBackward() const {
    std::vector<int> out;
    for (auto it = m_tracks.crbegin(); it != m_tracks.crend(); ++it)
        out.push_back(it->code);
    return out;
}

Result Program::replaceAll(std::deque<Track> tracks) {
    if (Result r = validateAll(tracks); !r)
        return r;
    for (Track& t : tracks) {
        t.title = t.title.trimmed();
        t.artist = t.artist.trimmed();
    }
    m_tracks = std::move(tracks);
    return Result::success();
}

}  // namespace music
