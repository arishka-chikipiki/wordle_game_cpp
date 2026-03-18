#include "gamewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QListWidget>
#include <QFile>
#include <QTextStream>
#include <algorithm>

GameWindow::GameWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Слово из 4 букв");
    resize(540, 460);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout *topLayout = new QHBoxLayout();
    btnGame    = new QPushButton("Меню игры");
    btnRecords = new QPushButton("Меню рекордов");
    btnNewGame = new QPushButton("Новая игра");

    btnGame->setCheckable(true);
    btnRecords->setCheckable(true);
    btnGame->setChecked(true);

    topLayout->addWidget(btnGame);
    topLayout->addWidget(btnRecords);
    topLayout->addStretch();
    topLayout->addWidget(btnNewGame);
    mainLayout->addLayout(topLayout);

    stack = new QStackedWidget();
    mainLayout->addWidget(stack, 1);

    // Страница игры
    QWidget *gamePage = new QWidget();
    QVBoxLayout *gameLay = new QVBoxLayout(gamePage);

    log = new QTextEdit();
    log->setReadOnly(true);

    input = new QLineEdit();
    input->setMaxLength(4);
    input->setPlaceholderText("введите 4 буквы");

    btnCheck = new QPushButton("Проверить");

    QHBoxLayout *inputLay = new QHBoxLayout();
    inputLay->addWidget(input);
    inputLay->addWidget(btnCheck);

    gameLay->addWidget(new QLabel("Игра:"));
    gameLay->addWidget(log, 1);
    gameLay->addLayout(inputLay);

    stack->addWidget(gamePage);

    // Страница рекордов
    QWidget *recordsPage = new QWidget();
    QVBoxLayout *recLay = new QVBoxLayout(recordsPage);
    recLay->addWidget(new QLabel("Лучшие результаты:"));
    listRecords = new QListWidget();
    recLay->addWidget(listRecords);

    stack->addWidget(recordsPage);

    // Слова
    words << "барс" << "лось" ;

    loadScores();

    input->setEnabled(false);
    btnCheck->setEnabled(false);

    connect(btnGame,    &QPushButton::clicked, this, &GameWindow::showGamePage);
    connect(btnRecords, &QPushButton::clicked, this, &GameWindow::showRecordsPage);
    connect(btnNewGame, &QPushButton::clicked, this, &GameWindow::startNewGame);
    connect(btnCheck,   &QPushButton::clicked, this, &GameWindow::checkWord);
    connect(input,      &QLineEdit::returnPressed, this, &GameWindow::checkWord);
}

// ────────────────────────────────────────────────

void GameWindow::showGamePage()
{
    btnGame->setChecked(true);
    btnRecords->setChecked(false);
    stack->setCurrentIndex(0);
}

void GameWindow::showRecordsPage()
{
    btnGame->setChecked(false);
    btnRecords->setChecked(true);
    stack->setCurrentIndex(1);
}

void GameWindow::startNewGame()
{
    // Сохраняем предыдущий результат ПЕРЕД сбросом, если он был > 0
    if (logic.sessionScore > 0)
    {
        saveScore(logic.sessionScore);
        log->append(QString("\nПредыдущая сессия сохранена: %1 баллов").arg(logic.sessionScore));
    }

    // теперь сбрасываем и начинаем новую
    logic.sessionScore = 0;
    logic.newWord(words);

    log->clear();
    log->append("Новая игра начата");
    log->append("Загадано слово: ****");

    waitingForContinue = false;
    input->clear();
    input->setEnabled(true);
    btnCheck->setEnabled(true);
    input->setFocus();
}
void GameWindow::saveScore(int score)
{
    if (score <= 0) return;

    // Добавляем новую запись
    int playerNumber = recordLines.size() + 1;
    QString newLine = QString("игрок %1: %2 баллов").arg(playerNumber).arg(score);
    recordLines.append(newLine);
    scores.append(score);

    // Сортируем сразу
    sortRecords();

    // Перезаписываем файл в отсортированном виде
    QFile file("scores.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    for (const QString &line : recordLines) {
        out << line << "\n";
    }
    file.close();

    updateRecordsList();
}
void GameWindow::checkWord()
{
    QString guess = input->text().trimmed().toLower();

    if (waitingForContinue)
    {
        QString answer = guess.toLower();

        if (answer == "y" || answer == "yes" || answer == "да" || answer == "д")
        {
            log->append("→ да / продолжаем");
            logic.newWord(words);
            log->append("Новое слово загадано: ****");
            waitingForContinue = false;
        }
        else
        {
            log->append("→ нет / игра окончена");
            log->append(QString("Вы заработали %1 баллов за всю сессию.").arg(logic.sessionScore));
            saveScore(logic.sessionScore);
            updateRecordsList();
            waitingForContinue = false;
            input->setEnabled(false);
            btnCheck->setEnabled(false);
        }

        input->clear();
        return;
    }

    if (guess.length() != 4)
    {
        log->append("Ошибка: нужно ввести ровно 4 буквы");
        input->clear();
        return;
    }

    logic.attempts++;
    log->append("→ " + guess);
    input->clear();

    if (logic.isCorrect(guess))
    {
        int pts = logic.pointsForThisWin();
        logic.sessionScore += pts;

        log->append(QString("Вы отгадали слово и заработали %1 баллов. Хотите продолжить? (y/n)").arg(pts));

        waitingForContinue = true;
        input->setFocus();
        return;
    }

    if (logic.attempts >= 5)
    {
        log->append("Попытки закончились!");
        log->append("Слово было: " + logic.secretWord);
        log->append(QString("Счёт за сессию: %1 баллов").arg(logic.sessionScore));
        saveScore(logic.sessionScore);
        updateRecordsList();
        input->setEnabled(false);
        btnCheck->setEnabled(false);
        return;
    }

    log->append(logic.getHint(guess));
}

// ────────────────────────────────────────────────

void GameWindow::loadScores()
{
    QFile file("scores.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    recordLines.clear();
    scores.clear();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        int colon = line.indexOf(':');
        if (colon < 0) continue;

        QString numStr = line.mid(colon + 1).trimmed().split(' ').first();
        bool ok;
        int val = numStr.toInt(&ok);
        if (ok && val > 0) {
            recordLines.append(line);
            scores.append(val);
        }
    }

    file.close();

    sortRecords();           // сортируем при загрузке
    updateRecordsList();
}
void GameWindow::sortRecords()
{
    QVector<QPair<int, QString>> items;

    for (int i = 0; i < recordLines.size(); ++i) {
        QString line = recordLines[i];
        int colon = line.indexOf(':');
        if (colon < 0) continue;

        QString numStr = line.mid(colon + 1).trimmed().split(' ').first();
        bool ok;
        int pts = numStr.toInt(&ok);
        if (ok) {
            items.append({-pts, line});  // отрицательное для сортировки по убыванию
        }
    }

    std::sort(items.begin(), items.end());

    // Перезаписываем отсортированные списки
    recordLines.clear();
    scores.clear();

    for (const auto &p : items) {
        recordLines.append(p.second);
        scores.append(-p.first);
    }
}
void GameWindow::updateRecordsList()
{
    listRecords->clear();
    if (recordLines.isEmpty()) {
        listRecords->addItem("(пока нет рекордов)");
        return;
    }

    for (int i = 0; i < recordLines.size() && i < 10; ++i) {
        QString line = recordLines[i];
        int colon = line.indexOf(':');
        QString namePart = (colon >= 0) ? line.left(colon + 1) : line;
        QString pointsPart = (colon >= 0) ? line.mid(colon + 1).trimmed() : "";

        QString display = QString("%1. %2 %3")
                             .arg(i + 1)
                             .arg(namePart.trimmed())
                             .arg(pointsPart);

        listRecords->addItem(display);
    }
}
