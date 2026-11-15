#include "Storage.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cmath>

namespace music {

namespace {

bool readInt(const QJsonValue& v, int& out) {
    if (!v.isDouble())
        return false;
    const double d = v.toDouble();
    if (d != std::floor(d) || d < -2147483648.0 || d > 2147483647.0)
        return false;
    out = static_cast<int>(d);
    return true;
}

}  // namespace

QByteArray toJson(const Program& program) {
    QJsonArray arr;
    for (const Track& t : program.tracks()) {
        QJsonObject o;
        o["code"] = t.code;
        o["title"] = t.title;
        o["artist"] = t.artist;
        o["duration"] = t.duration;
        arr.append(o);
    }
    QJsonObject state;
    state["tracks"] = arr;
    QJsonObject root;
    root["formatVersion"] = kFormatVersion;
    root["variant"] = kVariant;
    root["state"] = state;
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

Result parseJson(const QByteArray& data, std::deque<Track>& out) {
    if (data.size() > kMaxFileSize)
        return Result::fail(QStringLiteral("Файл больше 1 МиБ."));
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return Result::fail(QStringLiteral("Файл не является корректным JSON-объектом."));
    const QJsonObject root = doc.object();

    int version = 0;
    if (!readInt(root.value("formatVersion"), version))
        return Result::fail(QStringLiteral("Нет целого поля formatVersion."));
    if (version != kFormatVersion)
        return Result::fail(QStringLiteral("Неподдерживаемая версия формата: %1.").arg(version));
    int variant = 0;
    if (!readInt(root.value("variant"), variant))
        return Result::fail(QStringLiteral("Нет целого поля variant."));
    if (variant != kVariant)
        return Result::fail(QStringLiteral("Файл от другого варианта задания: %1.").arg(variant));
    if (!root.value("state").isObject())
        return Result::fail(QStringLiteral("Нет объекта state."));
    const QJsonValue tracksValue = root.value("state").toObject().value("tracks");
    if (!tracksValue.isArray())
        return Result::fail(QStringLiteral("В state нет массива tracks."));

    std::deque<Track> tmp;
    int index = 0;
    for (const QJsonValue& item : tracksValue.toArray()) {
        ++index;
        if (!item.isObject())
            return Result::fail(QStringLiteral("Запись %1 не является объектом.").arg(index));
        const QJsonObject o = item.toObject();
        Track t;
        if (!readInt(o.value("code"), t.code) || !readInt(o.value("duration"), t.duration))
            return Result::fail(
                QStringLiteral("Запись %1: code и duration должны быть целыми.").arg(index));
        if (!o.value("title").isString() || !o.value("artist").isString())
            return Result::fail(
                QStringLiteral("Запись %1: title и artist должны быть строками.").arg(index));
        t.title = o.value("title").toString();
        t.artist = o.value("artist").toString();
        tmp.push_back(std::move(t));
        if (tmp.size() > static_cast<std::size_t>(kMaxTracks))
            return Result::fail(QStringLiteral("Превышен предел в 1000 номеров."));
    }
    if (Result r = Program::validateAll(tmp); !r)
        return r;
    out = std::move(tmp);
    return Result::success();
}

Result saveToFile(const QString& path, const Program& program) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return Result::fail(QStringLiteral("Не удалось открыть файл для записи: %1").arg(file.errorString()));
    const QByteArray data = toJson(program);
    if (file.write(data) != data.size())
        return Result::fail(QStringLiteral("Ошибка записи: %1").arg(file.errorString()));
    if (!file.commit())
        return Result::fail(QStringLiteral("Не удалось завершить запись: %1").arg(file.errorString()));
    return Result::success();
}

Result loadFromFile(const QString& path, Program& program) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return Result::fail(QStringLiteral("Не удалось открыть файл: %1").arg(file.errorString()));
    if (file.size() > kMaxFileSize)
        return Result::fail(QStringLiteral("Файл больше 1 МиБ."));
    const QByteArray data = file.read(kMaxFileSize + 1);
    std::deque<Track> tmp;
    if (Result r = parseJson(data, tmp); !r)
        return r;
    return program.replaceAll(std::move(tmp));
}

}  // namespace music
