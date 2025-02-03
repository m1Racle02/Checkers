#pragma once
#include <tuple>
#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс Hand отвечает за обработку действий пользователя (например, кликов мыши) и предоставляет интерфейс для взаимодействия с доской.
class Hand
{
public:
    // Конструктор класса Hand. Принимает указатель на объект Board для доступа к информации о игровом поле.
    Hand(Board* board) : board(board)
    {
    }

    // Метод get_cell() обрабатывает события ввода пользователя и возвращает выбранную ячейку или действие.
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Переменная для хранения событий SDL.
        Response resp = Response::OK; // Инициализация переменной для хранения типа ответа.
        int x = -1, y = -1; // Координаты курсора мыши.
        int xc = -1, yc = -1; // Преобразованные координаты клетки на игровом поле.

        // Цикл для обработки событий SDL.
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие событий SDL.
            {
                switch (windowEvent.type) // Обработка различных типов событий.
                {
                case SDL_QUIT: // Если пользователь закрыл окно.
                    resp = Response::QUIT; // Устанавливаем ответ QUIT.
                    break;

                case SDL_MOUSEBUTTONDOWN: // Если пользователь нажал кнопку мыши.
                    x = windowEvent.motion.x; // Получаем текущие координаты курсора.
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1); // Преобразуем координаты курсора в индексы клеток.
                    yc = int(x / (board->W / 10) - 1);

                    // Обработка специальных зон:
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1) // Проверка на отмену хода.
                    {
                        resp = Response::BACK; // Устанавливаем ответ BACK.
                    }
                    else if (xc == -1 && yc == 8) // Проверка на повтор игры.
                    {
                        resp = Response::REPLAY; // Устанавливаем ответ REPLAY.
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8) // Проверка на выбор клетки внутри игрового поля.
                    {
                        resp = Response::CELL; // Устанавливаем ответ CELL.
                    }
                    else
                    {
                        xc = -1; // Сбрасываем координаты, если они находятся вне допустимой области.
                        yc = -1;
                    }
                    break;

                case SDL_WINDOWEVENT: // Обработка событий изменения размера окна.
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // Обновляем размеры окна.
                        break;
                    }
                }

                if (resp != Response::OK) // Если ответ отличен от OK, выходим из цикла.
                    break;
            }
        }

        // Возвращаем результат в виде кортежа: тип ответа и координаты клетки.
        return { resp, xc, yc };
    }

    // Метод wait() ожидает действия пользователя после завершения игры (например, повтор или выход).
    Response wait() const
    {
        SDL_Event windowEvent; // Переменная для хранения событий SDL.
        Response resp = Response::OK; // Инициализация переменной для хранения типа ответа.

        // Цикл для обработки событий SDL.
        while (true)
        {
            if (SDL_PollEvent(&windowEvent)) // Проверяем наличие событий SDL.
            {
                switch (windowEvent.type) // Обработка различных типов событий.
                {
                case SDL_QUIT: // Если пользователь закрыл окно.
                    resp = Response::QUIT; // Устанавливаем ответ QUIT.
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED: // Обработка события изменения размера окна.
                    board->reset_window_size(); // Обновляем размеры окна.
                    break;

                case SDL_MOUSEBUTTONDOWN: // Если пользователь нажал кнопку мыши.
                {
                    int x = windowEvent.motion.x; // Получаем текущие координаты курсора.
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1); // Преобразуем координаты курсора в индексы клеток.
                    int yc = int(x / (board->W / 10) - 1);

                    if (xc == -1 && yc == 8) // Проверка на запрос повторной игры.
                        resp = Response::REPLAY; // Устанавливаем ответ REPLAY.
                }
                break;
                }

                if (resp != Response::OK) // Если ответ отличен от OK, выходим из цикла.
                    break;
            }
        }

        // Возвращаем результат.
        return resp;
    }

private:
    Board* board; // Указатель на объект Board для доступа к информации о игровом поле.
};