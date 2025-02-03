#pragma once
#include <chrono>
#include <thread>
#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        // Создание объектов игры: игрового поля (board), управления ходами (hand) и логики игры (logic).
        // Также создается файл log.txt для записи логов игры.
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Основная функция для запуска игры.
    int play()
    {
        auto start = chrono::steady_clock::now(); // Запись времени начала игры для расчета общей длительности.

        if (is_replay)
        {
            // Если игра повторяется (replay), пересоздаем объект логики и перезагружаем настройки.
            logic = Logic(&board, &config);
            config.reload(); // Перезагрузка конфигурации из файла settings.json.
            board.redraw(); // Перерисовка игрового поля.
        }
        else
        {
            // Если это новая игра, начинаем рисовать доску с нуля.
            board.start_draw();
        }

        is_replay = false; // Сбрасываем флаг повторной игры.
        int turn_num = -1; // Номер текущего хода (начинается с -1, чтобы первый ход был 0).
        bool is_quit = false; // Флаг выхода из игры.
        const int Max_turns = config("Game", "MaxNumTurns"); // Максимальное количество ходов из конфигурации.

        // Главный цикл игры, который выполняется до достижения максимального количества ходов или завершения игры.
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // Сброс серии ударов (beats).

            // Поиск возможных ходов для текущего игрока (цвет определяется по номеру хода: 0 — белые, 1 — черные).
            logic.find_turns(turn_num % 2);

            // Если нет доступных ходов, выходим из цикла (игра завершена).
            if (logic.turns.empty())
                break;

            // Установка глубины поиска для бота в зависимости от текущего игрока.
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Проверяем, является ли текущий игрок человеком или ботом.
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Игрок — человек. Вызываем функцию player_turn для выполнения хода.
                auto resp = player_turn(turn_num % 2);

                if (resp == Response::QUIT)
                {
                    // Если игрок выбрал выход, устанавливаем флаг выхода.
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    // Если игрок запросил повторную игру, устанавливаем флаг replay.
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Если игрок запросил отмену хода:
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        // Откатываем ход, если противник — бот и не было серии ударов.
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;
                    board.rollback(); // Откатываем последний ход.
                    --turn_num;
                    beat_series = 0; // Сбрасываем серию ударов.
                }
            }
            else
            {
                // Игрок — бот. Выполняем ход бота.
                bot_turn(turn_num % 2);
            }
        }

        // Запись времени окончания игры для расчета общей длительности.
        auto end = chrono::steady_clock::now();

        // Записываем время игры в лог-файл.
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        // Если была запрошена повторная игра, вызываем play() снова.
        if (is_replay)
            return play();

        // Если был выполнен выход из игры, завершаем функцию.
        if (is_quit)
            return 0;

        // Определяем результат игры:
        int res = 2; // По умолчанию ничья.
        if (turn_num == Max_turns)
        {
            res = 0; // Максимальное количество ходов достигнуто — ничья.
        }
        else if (turn_num % 2)
        {
            res = 1; // Чёрные победили.
        }
        else
        {
            res = -1; // Белые победили.
        }

        // Показываем результат игры на экране.
        board.show_final(res);

        // Ждём реакцию игрока после окончания игры.
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            // Если игрок запросил повторную игру, вызываем play() снова.
            is_replay = true;
            return play();
        }

        return res; // Возвращаем результат игры.
    }

private:
    void bot_turn(const bool color)
    {
        // Функция для выполнения хода ботом.
        auto start = chrono::steady_clock::now(); // Запись времени начала хода бота.
        auto delay_ms = config("Bot", "BotDelayMS"); // Задержка перед ходом бота.

        // Создаем новый поток для задержки перед ходом бота.
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color); // Находим лучшие ходы для бота.
        th.join(); // Дожидаемся завершения задержки.

        bool is_first = true; // Флаг первого хода в серии.
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms); // Добавляем задержку между ходами, если их несколько.
            }
            is_first = false;

            // Увеличиваем счетчик серий ударов, если ход включает удар.
            beat_series += (turn.xb != -1);

            // Выполняем ход на доске.
            board.move_piece(turn, beat_series);
        }

        // Записываем время хода бота в лог-файл.
        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // Функция для выполнения хода игроком.
        vector<pair<POS_T, POS_T>> cells; // Список возможных ячеек для выбора.

        // Подсвечиваем все возможные ячейки для хода.
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        board.highlight_cells(cells);

        move_pos pos = { -1, -1, -1, -1 }; // Инициализация позиции хода.
        POS_T x = -1, y = -1; // Координаты выбранной фигуры.

        // Цикл для выбора первой ячейки и выполнения хода.
        while (true)
        {
            auto resp = hand.get_cell(); // Получаем выбор игрока.
            if (get<0>(resp) != Response::CELL)
            {
                // Если игрок выбрал действие, отличное от выбора клетки (например, выход или отмена), возвращаем соответствующий ответ.
                return get<0>(resp);
            }

            pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) }; // Координаты выбранной клетки.

            bool is_correct = false; // Флаг корректности выбора.

            // Проверяем, является ли выбранная клетка частью возможных ходов.
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }

            if (pos.x != -1)
            {
                break; // Если ход выбран корректно, завершаем цикл.
            }

            if (!is_correct)
            {
                // Если клетка некорректна, очищаем подсветку и показываем доступные варианты.
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }

            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);

            // Подсвечиваем возможные ходы для выбранной фигуры.
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        // Очищаем подсветку и выполняем ход.
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);

        if (pos.xb == -1)
        {
            return Response::OK; // Если ход выполнен без удара, завершаем функцию.
        }

        // Продолжаем серию ударов, если это возможно.
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2); // Находим возможные ходы после удара.

            if (!logic.have_beats)
            {
                break; // Если больше нельзя ударить, завершаем цикл.
            }

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);

            // Выбор следующего хода.
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                {
                    return get<0>(resp);
                }

                pair<POS_T, POS_T> cell{ get<1>(resp), get<2>(resp) };
                bool is_correct = false;

                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }

                if (!is_correct)
                {
                    continue; // Если выбор некорректен, продолжаем цикл.
                }

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK; // Ход выполнен успешно.
    }

private:
    Config config; // Объект для работы с конфигурацией игры.
    Board board; // Объект игрового поля.
    Hand hand; // Объект для управления действиями игрока.
    Logic logic; // Объект для логики игры.
    int beat_series; // Счетчик серий ударов.
    bool is_replay = false; // Флаг повторной игры.
};