#pragma once

#include <random>
#include <vector>
#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

// Константа для представления бесконечности в расчетах
const int INF = 1e9;
class Logic {
public:
    // Конструктор класса
    Logic(Board* board, Config* config) : board(board), config(config) {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    // Основная функция для поиска лучших ходов
    vector<move_pos> find_best_turns(const bool color) {
        next_best_state.clear(); // Очищаем список следующих состояний
        next_move.clear();       // Очищаем список следующих ходов

        // Начинаем поиск первого лучшего хода
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0; // Текущее состояние (начинаем с корня дерева)
        vector<move_pos> res; // Результирующий вектор ходов

        // Собираем последовательность ходов из найденных состояний
        do {
            res.push_back(next_move[cur_state]); // Добавляем текущий ход в результат
            cur_state = next_best_state[cur_state]; // Переходим к следующему состоянию
        } while (cur_state != -1 && next_move[cur_state].x != -1); // Продолжаем, пока не достигнем конца цепочки

        return res; // Возвращаем результирующую последовательность ходов
    }

private:
    // Рекурсивный поиск первого лучшего хода
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1) {
        next_best_state.push_back(-1); // Добавляем новое состояние в список
        next_move.emplace_back(-1, -1, -1, -1); // Добавляем новый ход (пустой)

        double best_score = -1; // Лучшая оценка пока отсутствует

        if (state != 0) { // Если это не корень дерева, ищем возможные ходы
            find_turns(x, y, mtx);
        }

        auto turns_now = turns; // Сохраняем текущие ходы
        bool have_beats_now = have_beats; // Сохраняем флаг наличия ударов

        if (!have_beats_now && state != 0) { // Если нет ударов, переходим к следующему уровню
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        vector<move_pos> best_moves; // Хранение лучших ходов
        vector<int> best_states;     // Хранение соответствующих состояний

        for (auto turn : turns_now) { // Перебор всех возможных ходов
            size_t next_state = next_move.size(); // Индекс следующего состояния

            double score;
            if (have_beats_now) { // Если есть удары, продолжаем цепочку
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else { // Если нет ударов, переходим к следующему уровню
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            if (score > best_score) { // Обновляем лучший ход
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1); // Сохраняем следующее состояние
                next_move[state] = turn; // Сохраняем ход
            }
        }

        return best_score; // Возвращаем лучшую оценку
    }

    // Рекурсивный поиск лучших ходов с использованием минимакса
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
        if (depth == Max_depth) { // Если достигнута максимальная глубина, возвращаем оценку позиции
            return calc_score(mtx, (depth % 2 == color));
        }

        if (x != -1) { // Если указаны конкретные координаты, ищем ходы только для них
            find_turns(x, y, mtx);
        }
        else { // Иначе ищем ходы для всего игрока
            find_turns(color, mtx);
        }

        auto turns_now = turns; // Сохраняем текущие ходы
        bool have_beats_now = have_beats; // Сохраняем флаг наличия ударов

        if (!have_beats_now && x != -1) { // Если нет ударов, переходим к следующему уровню
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        if (turns.empty()) { // Если ходов нет, возвращаем соответствующую оценку
            return (depth % 2 ? 0 : INF);
        }

        double min_score = INF + 1; // Минимальная оценка
        double max_score = -1;      // Максимальная оценка

        for (auto turn : turns_now) { // Перебор всех возможных ходов
            double score = 0.0;

            if (!have_beats_now && x == -1) { // Если нет ударов и не указаны координаты
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else { // Иначе продолжаем цепочку ходов
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }

            min_score = min(min_score, score); // Обновляем минимальную оценку
            max_score = max(max_score, score); // Обновляем максимальную оценку

            // Альфа-бета отсечение
            if (depth % 2) { // Если текущий уровень принадлежит максимизирующему игроку
                alpha = max(alpha, max_score);
            }
            else { // Если текущий уровень принадлежит минимизирующему игроку
                beta = min(beta, min_score);
            }

            if (optimization != "O0" && alpha >= beta) { // Если выполнено условие отсечения
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }

        return (depth % 2 ? max_score : min_score); // Возвращаем результат
    }

public:
    // Перегруженные функции для поиска ходов
    void find_turns(const bool color) {
        find_turns(color, board->get_board()); // Ищем ходы для заданного цвета на текущей доске
    }

    void find_turns(const POS_T x, const POS_T y) {
        find_turns(x, y, board->get_board()); // Ищем ходы для фигуры в заданной позиции на текущей доске
    }

private:
    // Основная функция для поиска ходов для игрока
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx) {
        vector<move_pos> res_turns; // Результирующий вектор ходов
        bool have_beats_before = false; // Флаг наличия ударов до текущего перебора
        for (POS_T i = 0; i < 8; ++i) {
            for (POS_T j = 0; j < 8; ++j) {
                // Если клетка содержит фигуру противоположного цвета, пропускаем
                if (mtx[i][j] && mtx[i][j] % 2 != color) {
                    find_turns(i, j, mtx); // Ищем ходы для фигуры в данной позиции
                    if (have_beats && !have_beats_before) { // Если появились удары, очищаем предыдущие ходы
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before) { // Добавляем ходы
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns; // Обновляем список ходов
        shuffle(turns.begin(), turns.end(), rand_eng); // Перемешиваем ходы для случайности
        have_beats = have_beats_before; // Обновляем флаг наличия ударов
    }

    // Функция для поиска ходов для конкретной фигуры
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx) {
        turns.clear(); // Очищаем предыдущие ходы
        have_beats = false; // Сбрасываем флаг наличия ударов
        POS_T type = mtx[x][y]; // Тип фигуры
        // Проверяем возможность ударов
        switch (type) {
        case 1: // Белая шашка
        case 2: // Черная шашка
            // Проверяем соседние диагонали для возможности удара
            for (POS_T i = x - 2; i <= x + 2; i += 4) {
                for (POS_T j = y - 2; j <= y + 2; j += 4) {
                    if (i < 0 || i > 7 || j < 0 || j > 7) // Проверяем границы доски
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2; // Координаты побитой фигуры
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2) // Проверяем условия удара
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb); // Добавляем ход
                }
            }
            break;
        default: // Дамка
            // Проверяем все направления для дамки
            for (POS_T i = -1; i <= 1; i += 2) {
                for (POS_T j = -1; j <= 1; j += 2) {
                    POS_T xb = -1, yb = -1; // Координаты побитой фигуры
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j) {
                        if (mtx[i2][j2]) { // Если встретили фигуру
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1)) {
                                break; // Прерываем, если фигура своего цвета или уже была побита
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2) { // Если можно побить
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // Если удары найдены, завершаем
        if (!turns.empty()) {
            have_beats = true;
            return;
        }
        // Проверяем обычные ходы
        switch (type) {
        case 1: // Белая шашка
        case 2: // Черная шашка
            POS_T i = ((type % 2) ? x - 1 : x + 1); // Направление движения
            for (POS_T j = y - 1; j <= y + 1; j += 2) { // Диагональные ходы
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j]) // Проверяем границы и занятость клетки
                    continue;
                turns.emplace_back(x, y, i, j); // Добавляем ход
            }
            break;
        default: // Дамка
            // Проверяем все направления для дамки
            for (POS_T i = -1; i <= 1; i += 2) {
                for (POS_T j = -1; j <= 1; j += 2) {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j) {
                        if (mtx[i2][j2]) // Если клетка занята, прерываем
                            break;
                        turns.emplace_back(x, y, i2, j2); // Добавляем ход
                    }
                }
            }
            break;
        }
    }

public:
    // Публичные поля класса

    vector<move_pos> turns; // Все найденные ходы
    bool have_beats;        // Флаг наличия ударов
    int Max_depth;          // Максимальная глубина поиска

private:
    // Приватные поля класса

    default_random_engine rand_eng; // Генератор случайных чисел
    string scoring_mode;           // Режим оценки позиции
    string optimization;           // Уровень оптимизации алгоритма
    vector<move_pos> next_move;    // Последовательность следующих ходов
    vector<int> next_best_state;   // Последовательность следующих состояний
    Board* board;                  // Указатель на игровую доску
    Config* config;                // Указатель на конфигурацию игры
};