#include "MainWindow.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

#include "Storage.h"

namespace {
QString formatDuration(long long s) {
    return QStringLiteral("%1:%2:%3")
        .arg(s / 3600)
        .arg((s % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(s % 60, 2, 10, QLatin1Char('0'));
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Музыкальная программа"));
    resize(900, 600);

    m_code = new QLineEdit;
    m_title = new QLineEdit;
    m_artist = new QLineEdit;
    m_duration = new QLineEdit;
    m_search = new QLineEdit;
    m_k = new QLineEdit;
    m_code->setPlaceholderText(QStringLiteral("Код"));
    m_title->setPlaceholderText(QStringLiteral("Название"));
    m_artist->setPlaceholderText(QStringLiteral("Исполнитель"));
    m_duration->setPlaceholderText(QStringLiteral("Секунды"));
    m_search->setPlaceholderText(QStringLiteral("Фрагмент названия"));
    m_k->setPlaceholderText(QStringLiteral("k"));

    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Код"), QStringLiteral("Название"),
                                        QStringLiteral("Исполнитель"),
                                        QStringLiteral("Секунды")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    m_summary = new QLabel;
    m_status = new QLabel(QStringLiteral("Готово."));

    auto button = [this](const QString& text, auto handler) {
        auto* b = new QPushButton(text);
        connect(b, &QPushButton::clicked, this, handler);
        return b;
    };

    auto* form = new QHBoxLayout;
    for (QLineEdit* e : {m_code, m_title, m_artist, m_duration})
        form->addWidget(e);

    auto* cmds = new QHBoxLayout;
    cmds->addWidget(button(QStringLiteral("Добавить в начало"), [this] {
        music::Track t;
        if (readTrack(t))
            finish(m_program.addFront(t), QStringLiteral("Номер добавлен в начало."));
    }));
    cmds->addWidget(button(QStringLiteral("Добавить в конец"), [this] {
        music::Track t;
        if (readTrack(t))
            finish(m_program.addBack(t), QStringLiteral("Номер добавлен в конец."));
    }));
    cmds->addWidget(button(QStringLiteral("Отменить первый"), [this] {
        finish(m_program.removeFront(), QStringLiteral("Первый номер удалён."));
    }));
    cmds->addWidget(button(QStringLiteral("Отменить последний"), [this] {
        finish(m_program.removeBack(), QStringLiteral("Последний номер удалён."));
    }));

    auto* row2 = new QHBoxLayout;
    row2->addWidget(m_search);
    row2->addWidget(button(QStringLiteral("Найти"), [this] {
        m_filter = m_search->text();
        refresh();
        m_status->setText(QStringLiteral("Поиск выполнен."));
    }));
    row2->addWidget(button(QStringLiteral("Показать всё"), [this] {
        m_filter.clear();
        m_search->clear();
        refresh();
        m_status->setText(QStringLiteral("Показаны все номера."));
    }));
    row2->addWidget(button(QStringLiteral("Обратный порядок"), [this] {
        m_reversed = !m_reversed;
        refresh();
        m_status->setText(m_reversed ? QStringLiteral("Просмотр с конца.")
                                     : QStringLiteral("Обычный просмотр."));
    }));

    auto* row3 = new QHBoxLayout;
    row3->addWidget(m_k);
    row3->addWidget(button(QStringLiteral("Перенести первые k в конец"), [this] {
        int k = 0;
        if (readInt(m_k, QStringLiteral("k"), k))
            finish(m_program.rotateFirst(k), QStringLiteral("Перенос выполнен."));
    }));
    row3->addWidget(button(QStringLiteral("Изменить продолжительность по коду"), [this] {
        int code = 0, sec = 0;
        if (readInt(m_code, QStringLiteral("Код"), code) &&
            readInt(m_duration, QStringLiteral("Секунды"), sec))
            finish(m_program.setDuration(code, sec), QStringLiteral("Продолжительность изменена."));
    }));

    auto* row4 = new QHBoxLayout;
    row4->addWidget(button(QStringLiteral("Сохранить"), [this] {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Сохранить"), {},
                                                          QStringLiteral("JSON (*.json)"));
        if (!path.isEmpty())
            finish(music::saveToFile(path, m_program), QStringLiteral("Файл сохранён."));
    }));
    row4->addWidget(button(QStringLiteral("Открыть"), [this] {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Открыть"), {},
                                                          QStringLiteral("JSON (*.json)"));
        if (!path.isEmpty())
            finish(music::loadFromFile(path, m_program), QStringLiteral("Файл открыт."));
    }));

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->addLayout(form);
    layout->addLayout(cmds);
    layout->addLayout(row2);
    layout->addLayout(row3);
    layout->addWidget(m_table);
    layout->addWidget(m_summary);
    layout->addLayout(row4);
    layout->addWidget(m_status);
    setCentralWidget(central);
    refresh();
}

bool MainWindow::readInt(QLineEdit* edit, const QString& what, int& value) {
    bool ok = false;
    const int v = edit->text().trimmed().toInt(&ok);
    if (!ok) {
        m_status->setText(QStringLiteral("Ошибка: поле «%1» должно быть целым числом.").arg(what));
        return false;
    }
    value = v;
    return true;
}

bool MainWindow::readTrack(music::Track& t) {
    if (!readInt(m_code, QStringLiteral("Код"), t.code) ||
        !readInt(m_duration, QStringLiteral("Секунды"), t.duration))
        return false;
    t.title = m_title->text();
    t.artist = m_artist->text();
    return true;
}

void MainWindow::finish(const music::Result& r, const QString& okMessage) {
    if (r)
        m_status->setText(okMessage);
    else
        m_status->setText(QStringLiteral("Ошибка: ") + r.error);
    refresh();
}

void MainWindow::refresh() {
    std::vector<music::Track> rows = m_program.search(m_filter);
    if (m_reversed)
        std::reverse(rows.begin(), rows.end());
    m_table->setRowCount(static_cast<int>(rows.size()));
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const music::Track& t = rows[static_cast<std::size_t>(i)];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(t.code)));
        m_table->setItem(i, 1, new QTableWidgetItem(t.title));
        m_table->setItem(i, 2, new QTableWidgetItem(t.artist));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(t.duration)));
    }
    m_summary->setText(QStringLiteral("Номеров: %1 (показано %2). Общая длительность: %3 с (%4)")
                           .arg(m_program.size())
                           .arg(rows.size())
                           .arg(m_program.totalDuration())
                           .arg(formatDuration(m_program.totalDuration())));
}
