#include "gamelogic.h"
#include <QRandomGenerator>

GameLogic::GameLogic()
    : attempts(0), sessionScore(0)
{
}

void GameLogic::newWord(const QStringList &wordList)
{
    if (wordList.isEmpty()) {
        secretWord = "abcd";
        return;
    }

    int idx = QRandomGenerator::global()->bounded(wordList.size());
    secretWord = wordList.at(idx).toLower();
    attempts = 0;
}

bool GameLogic::isCorrect(const QString &guess) const
{
    return guess == secretWord;
}

int GameLogic::pointsForThisWin() const
{
    if (attempts == 1) return 5;
    if (attempts == 2) return 4;
    if (attempts == 3) return 3;
    if (attempts == 4) return 2;
    return 1;
}

QString GameLogic::getHint(const QString &guess) const
{
    QString shown;
    QString letters;

    for (int i = 0; i < 4; ++i) {
        QChar g = guess.at(i);
        QChar s = secretWord.at(i);

        if (g == s) {
            shown += g;
            if (!letters.contains(g)) {
                if (!letters.isEmpty()) letters += ", ";
                letters += g;
            }
        }
        else if (secretWord.contains(g)) {
            shown += '_';
            if (!letters.contains(g)) {
                if (!letters.isEmpty()) letters += ", ";
                letters += g;
            }
        }
        else {
            shown += '_';
        }
    }

    QString letterPart = letters.isEmpty() ? "-" : letters;

    return "Отгаданные буквы: " + letterPart + "\nСлово: " + shown;
}
