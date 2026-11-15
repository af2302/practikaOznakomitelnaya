#pragma once
#include <QByteArray>
#include <QString>
#include <deque>

#include "Program.h"

namespace music {

inline constexpr int kFormatVersion = 1;
inline constexpr int kVariant = 2;
inline constexpr qint64 kMaxFileSize = 1024 * 1024;

// Формат: {"formatVersion":1,"variant":2,"state":{"tracks":[{code,title,artist,duration},...]}}
QByteArray toJson(const Program& program);
// Разбор и полная проверка. При ошибке out не изменяется.
Result parseJson(const QByteArray& data, std::deque<Track>& out);

Result saveToFile(const QString& path, const Program& program);
// Читает во временный объект и только после проверки заменяет состояние program.
Result loadFromFile(const QString& path, Program& program);

}  // namespace music
