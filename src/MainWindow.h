#pragma once
#include <QMainWindow>

#include "Program.h"

class QLabel;
class QLineEdit;
class QTableWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void refresh();
    void finish(const music::Result& r, const QString& okMessage);
    bool readTrack(music::Track& t);
    bool readInt(QLineEdit* edit, const QString& what, int& value);

    music::Program m_program;  // единственное хранилище данных
    bool m_reversed = false;
    QString m_filter;

    QLineEdit* m_code;
    QLineEdit* m_title;
    QLineEdit* m_artist;
    QLineEdit* m_duration;
    QLineEdit* m_search;
    QLineEdit* m_k;
    QTableWidget* m_table;
    QLabel* m_summary;
    QLabel* m_status;
};
