#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

#include "Program.h"
#include "Storage.h"

using namespace music;

namespace {
Track mk(int code, const QString& title = "T", int dur = 60) {
    return Track{code, title, QStringLiteral("Artist"), dur};
}
QString order(const Program& p) {
    QStringList l;
    for (int c : p.codesForward())
        l << QString::number(c);
    return l.join(',');
}
Program five() {
    Program p;
    for (int i = 1; i <= 5; ++i)
        p.addBack(mk(i));
    return p;
}
QByteArray doc(int version, int variant, const QString& tracks) {
    return QStringLiteral("{\"formatVersion\":%1,\"variant\":%2,\"state\":{\"tracks\":%3}}")
        .arg(version).arg(variant).arg(tracks).toUtf8();
}
}  // namespace

class TstMusic : public QObject {
    Q_OBJECT
private slots:
    void addFrontAndBack() {
        Program p;
        QVERIFY(p.addBack(mk(2)));
        QVERIFY(p.addFront(mk(1)));
        QVERIFY(p.addBack(mk(3)));
        QCOMPARE(order(p), QString("1,2,3"));
    }
    void removeFrontAndBack() {
        Program p = five();
        QVERIFY(p.removeFront());
        QVERIFY(p.removeBack());
        QCOMPARE(order(p), QString("2,3,4"));
    }
    void emptyState() {
        Program p;
        QVERIFY(p.empty());
        QCOMPARE(p.totalDuration(), 0LL);
        QVERIFY(!p.removeFront());
        QVERIFY(!p.removeBack());
        QVERIFY(p.search("").empty());
        QVERIFY(p.rotateFirst(0));
        QVERIFY(!p.rotateFirst(1));
    }
    void minimalNonEmpty() {
        Program p;
        QVERIFY(p.addBack(mk(7)));
        QCOMPARE(p.size(), std::size_t(1));
        QVERIFY(p.rotateFirst(1));
        QCOMPARE(order(p), QString("7"));
    }
    void limit1000() {
        Program p;
        for (int i = 1; i <= kMaxTracks; ++i)
            QVERIFY(p.addBack(mk(i)));
        QCOMPARE(p.size(), std::size_t(1000));
        Result r = p.addBack(mk(1001));
        QVERIFY(!r);
        QVERIFY(!r.error.isEmpty());
        QCOMPARE(p.size(), std::size_t(1000));
    }
    void invalidParametersKeepState() {
        Program p = five();
        const QString before = order(p);
        QVERIFY(!p.addBack(mk(0)));
        QVERIFY(!p.addBack(mk(kMaxCode + 1)));
        QVERIFY(!p.addBack(mk(10, "   ")));
        QVERIFY(!p.addBack(mk(10, QString(101, 'a'))));
        QVERIFY(!p.addBack(mk(10, "T", 0)));
        QVERIFY(!p.addBack(mk(10, "T", 86401)));
        QVERIFY(!p.addBack(mk(3)));  // повторный код
        Track noArtist = mk(10);
        noArtist.artist = "  ";
        QVERIFY(!p.addFront(noArtist));
        QCOMPARE(order(p), before);
    }
    void durationBoundsAccepted() {
        Program p;
        QVERIFY(p.addBack(mk(1, "A", 1)));
        QVERIFY(p.addBack(mk(2, "B", 86400)));
        QCOMPARE(p.totalDuration(), 86401LL);
    }
    void rotateExample() {
        Program p = five();
        QVERIFY(p.rotateFirst(2));
        QCOMPARE(order(p), QString("3,4,5,1,2"));
    }
    void rotateBoundaries() {
        Program p = five();
        QVERIFY(p.rotateFirst(0));
        QCOMPARE(order(p), QString("1,2,3,4,5"));
        QVERIFY(p.rotateFirst(5));
        QCOMPARE(order(p), QString("1,2,3,4,5"));
        QVERIFY(!p.rotateFirst(6));
        QVERIFY(!p.rotateFirst(-1));
        QCOMPARE(order(p), QString("1,2,3,4,5"));
    }
    void searchCaseInsensitiveKeepsOrder() {
        Program p;
        p.addBack(mk(1, "Вальс"));
        p.addBack(mk(2, "Танго"));
        p.addBack(mk(3, "БОЛЬШОЙ вальс"));
        p.addBack(mk(4, "Вальс"));  // названия могут повторяться
        auto r = p.search("вАЛЬС");
        QCOMPARE(r.size(), std::size_t(3));
        QCOMPARE(r[0].code, 1);
        QCOMPARE(r[1].code, 3);
        QCOMPARE(r[2].code, 4);
        QCOMPARE(p.search("").size(), std::size_t(4));
        QCOMPARE(p.size(), std::size_t(4));
        QVERIFY(p.search("нет такого").empty());
    }
    void forwardAndBackward() {
        Program p = five();
        auto f = p.codesForward();
        auto b = p.codesBackward();
        QCOMPARE(f.front(), 1);
        QCOMPARE(b.front(), 5);
        std::reverse(b.begin(), b.end());
        QCOMPARE(f, b);
        QCOMPARE(order(p), QString("1,2,3,4,5"));
    }
    void totalDurationAndSetDuration() {
        Program p;
        p.addBack(mk(1, "A", 60));
        p.addBack(mk(2, "B", 90));
        p.addBack(mk(3, "C", 30));
        QCOMPARE(p.totalDuration(), 180LL);
        QVERIFY(p.removeBack());
        QCOMPARE(p.totalDuration(), 150LL);
        QVERIFY(p.setDuration(2, 100));
        QCOMPARE(p.totalDuration(), 160LL);
        Result r = p.setDuration(99, 10);
        QVERIFY(!r);
        QVERIFY(r.error.contains("99"));
        QVERIFY(!p.setDuration(1, 0));
        QCOMPARE(p.totalDuration(), 160LL);
    }
    void saveLoadRoundTrip() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("s.json");
        Program p;
        p.addBack(mk(5, "Кто-то", 61));
        p.addFront(mk(9, "Другой", 7));
        p.addBack(mk(2, "Ещё", 3));
        QVERIFY(saveToFile(path, p));
        Program q;
        q.addBack(mk(100));
        QVERIFY(loadFromFile(path, q));
        QCOMPARE(order(q), QString("9,5,2"));
        QCOMPARE(q.tracks()[1].title, QString("Кто-то"));
        QCOMPARE(q.tracks()[1].duration, 61);
        QCOMPARE(q.totalDuration(), p.totalDuration());
    }
    void rejectBrokenJson() {
        std::deque<Track> out{mk(1)};
        QVERIFY(!parseJson("{not json", out));
        QVERIFY(!parseJson("[]", out));
        QCOMPARE(out.size(), std::size_t(1));
    }
    void rejectVersionAndVariant() {
        std::deque<Track> out;
        QVERIFY(!parseJson(doc(2, 2, "[]"), out));
        QVERIFY(!parseJson(doc(1, 3, "[]"), out));
        QVERIFY(parseJson(doc(1, 2, "[]"), out));
    }
    void rejectWrongTypesAndDomain() {
        std::deque<Track> out;
        const QString good = "{\"code\":1,\"title\":\"A\",\"artist\":\"B\",\"duration\":10}";
        QVERIFY(parseJson(doc(1, 2, "[" + good + "]"), out));
        QVERIFY(!parseJson(doc(1, 2, "[" + good + "," + good + "]"), out));  // дубликат
        QVERIFY(!parseJson(doc(1, 2,
            "[{\"code\":1,\"title\":\"A\",\"artist\":\"B\",\"duration\":0}]"), out));
        QVERIFY(!parseJson(doc(1, 2,
            "[{\"code\":1.5,\"title\":\"A\",\"artist\":\"B\",\"duration\":5}]"), out));
        QVERIFY(!parseJson(doc(1, 2,
            "[{\"code\":1,\"title\":5,\"artist\":\"B\",\"duration\":5}]"), out));
        QVERIFY(!parseJson(doc(1, 2, "[{\"code\":1}]"), out));
        QCOMPARE(out.size(), std::size_t(1));
    }
    void failedLoadKeepsState() {
        QTemporaryDir dir;
        const QString path = dir.filePath("bad.json");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(doc(1, 2, "[{\"code\":1,\"title\":\"A\",\"artist\":\"B\",\"duration\":0}]"));
        f.close();
        Program p = five();
        QVERIFY(!loadFromFile(path, p));
        QCOMPARE(order(p), QString("1,2,3,4,5"));
        QVERIFY(!loadFromFile(dir.filePath("missing.json"), p));
        QCOMPARE(p.size(), std::size_t(5));
    }
    void rejectTooLargeFile() {
        std::deque<Track> out;
        QVERIFY(!parseJson(QByteArray(kMaxFileSize + 1, ' '), out));
    }
};

QTEST_APPLESS_MAIN(TstMusic)
#include "tst_music.moc"
