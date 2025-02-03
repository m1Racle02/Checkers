#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../Models/Move.h"
#include "../Models/Project_path.h"

// Включение необходимых заголовков для работы с SDL2 на разных платформах
#ifdef __APPLE__
#include <SDL2/SDL_syswm.h>
#include <TargetConditionals.h>
#else
#include <SDL2/SDL_syswm.h>
#endif

using namespace std;

class Board {
public:
    // Конструкторы класса
    Board() = default; // Пустой конструктор по умолчанию
    Board(const unsigned int W, const unsigned int H) : W(W), H(H) {} // Конструктор с заданием размеров окна

    // Инициализация начальной доски
    int start_draw() {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { // Инициализация SDL2
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }
        if (W == 0 || H == 0) { // Если размеры окна не указаны, используем размер экрана
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm)) { // Получаем разрешение экрана
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h); // Берем меньшую сторону экрана
            W -= W / 15; // Уменьшаем размер для лучшего отображения
            H = W; // Делаем окно квадратным
        }
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE); // Создание окна
        if (win == nullptr) { // Проверка создания окна
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // Создание рендера
        if (ren == nullptr) { // Проверка создания рендера
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }
        // Загрузка текстур для доски, фигур и кнопок
        board = IMG_LoadTexture(ren, board_path.c_str());
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str());
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str());
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str());
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str());
        back = IMG_LoadTexture(ren, back_path.c_str());
        replay = IMG_LoadTexture(ren, replay_path.c_str());
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay) { // Проверка загрузки текстур
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }
        SDL_GetRendererOutputSize(ren, &W, &H); // Получение реальных размеров окна
        make_start_mtx(); // Создание начальной матрицы доски
        rerender(); // Перерисовка доски
        return 0;
    }

    // Метод для перезапуска игры
    void redraw() {
        game_results = -1; // Сброс результата игры
        history_mtx.clear(); // Очистка истории ходов
        history_beat_series.clear(); // Очистка серии ударов
        make_start_mtx(); // Создание новой начальной матрицы
        clear_active(); // Сброс активной клетки
        clear_highlight(); // Сброс подсветки клеток
    }

    // Метод для перемещения фигуры
    void move_piece(move_pos turn, const int beat_series = 0) {
        if (turn.xb != -1) { // Если есть побитая фигура, удаляем её
            mtx[turn.xb][turn.yb] = 0;
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series); // Выполняем перемещение
    }

    // Метод для перемещения фигуры по координатам
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0) {
        if (mtx[i2][j2]) { // Проверка, что целевая клетка пуста
            throw runtime_error("final position is not empty, can't move");
        }
        if (!mtx[i][j]) { // Проверка, что начальная клетка содержит фигуру
            throw runtime_error("begin position is empty, can't move");
        }
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7)) { // Проверка превращения в дамку
            mtx[i][j] += 2;
        }
        mtx[i2][j2] = mtx[i][j]; // Перемещаем фигуру
        drop_piece(i, j); // Очищаем начальную клетку
        add_history(beat_series); // Добавляем ход в историю
    }

    // Метод для удаления фигуры с доски
    void drop_piece(const POS_T i, const POS_T j) {
        mtx[i][j] = 0; // Очищаем клетку
        rerender(); // Перерисовываем доску
    }

    // Метод для превращения фигуры в дамку
    void turn_into_queen(const POS_T i, const POS_T j) {
        if (mtx[i][j] == 0 || mtx[i][j] > 2) { // Проверка, что фигура может превратиться в дамку
            throw runtime_error("can't turn into queen in this position");
        }
        mtx[i][j] += 2; // Превращаем фигуру в дамку
        rerender(); // Перерисовываем доску
    }

    // Метод для получения текущей матрицы доски
    vector<vector<POS_T>> get_board() const {
        return mtx;
    }

    // Метод для подсветки клеток
    void highlight_cells(vector<pair<POS_T, POS_T>> cells) {
        for (auto pos : cells) { // Подсвечиваем указанные клетки
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1;
        }
        rerender(); // Перерисовываем доску
    }

    // Метод для очистки подсветки клеток
    void clear_highlight() {
        for (POS_T i = 0; i < 8; ++i) { // Сбрасываем подсветку всех клеток
            is_highlighted_[i].assign(8, 0);
        }
        rerender(); // Перерисовываем доску
    }

    // Метод для установки активной клетки
    void set_active(const POS_T x, const POS_T y) {
        active_x = x; // Устанавливаем координаты активной клетки
        active_y = y;
        rerender(); // Перерисовываем доску
    }

    // Метод для сброса активной клетки
    void clear_active() {
        active_x = -1; // Сбрасываем координаты активной клетки
        active_y = -1;
        rerender(); // Перерисовываем доску
    }

    // Метод для проверки, подсвечена ли клетка
    bool is_highlighted(const POS_T x, const POS_T y) {
        return is_highlighted_[x][y]; // Возвращаем состояние клетки
    }

    // Метод для отката хода
    void rollback() {
        auto beat_series = max(1, *(history_beat_series.rbegin())); // Получаем последнюю серию ударов
        while (beat_series-- && history_mtx.size() > 1) { // Откатываемся до начала серии
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin()); // Восстанавливаем предыдущее состояние доски
        clear_highlight(); // Сбрасываем подсветку
        clear_active(); // Сбрасываем активную клетку
    }

    // Метод для отображения результата игры
    void show_final(const int res) {
        game_results = res; // Устанавливаем результат игры
        rerender(); // Перерисовываем доску
    }

    // Метод для обновления размеров окна
    void reset_window_size() {
        SDL_GetRendererOutputSize(ren, &W, &H); // Обновляем размеры окна
        rerender(); // Перерисовываем доску
    }

    // Метод для завершения работы SDL2
    void quit() {
        SDL_DestroyTexture(board); // Освобождаем текстуры
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren); // Освобождаем рендер
        SDL_DestroyWindow(win); // Освобождаем окно
        SDL_Quit(); // Завершаем работу SDL2
    }

    // Деструктор класса
    ~Board() {
        if (win) quit(); // Вызываем метод quit при уничтожении объекта
    }

private:
    // Метод для добавления хода в историю
    void add_history(const int beat_series = 0) {
        history_mtx.push_back(mtx); // Сохраняем текущее состояние доски
        history_beat_series.push_back(beat_series); // Сохраняем серию ударов
    }

    // Метод для создания начальной матрицы доски
    void make_start_mtx() {
        for (POS_T i = 0; i < 8; ++i) { // Заполняем матрицу начальными значениями
            for (POS_T j = 0; j < 8; ++j) {
                mtx[i][j] = 0; // Все клетки пустые
                if (i < 3 && (i + j) % 2 == 1) { // Размещаем черные шашки
                    mtx[i][j] = 2;
                }
                if (i > 4 && (i + j) % 2 == 1) { // Размещаем белые шашки
                    mtx[i][j] = 1;
                }
            }
        }
        add_history(); // Добавляем начальное состояние в историю
    }

    // Метод для перерисовки доски
    void rerender() {
        SDL_RenderClear(ren); // Очищаем экран
        SDL_RenderCopy(ren, board, NULL, NULL); // Рисуем доску

        // Рисуем фигуры
        for (POS_T i = 0; i < 8; ++i) {
            for (POS_T j = 0; j < 8; ++j) {
                if (!mtx[i][j]) continue; // Пропускаем пустые клетки
                int wpos = W * (j + 1) / 10 + W / 120; // Вычисляем позицию фигуры
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 }; // Создаем прямоугольник для фигуры
                SDL_Texture* piece_texture;
                if (mtx[i][j] == 1) piece_texture = w_piece; // Выбираем текстуру для фигуры
                else if (mtx[i][j] == 2) piece_texture = b_piece;
                else if (mtx[i][j] == 3) piece_texture = w_queen;
                else piece_texture = b_queen;
                SDL_RenderCopy(ren, piece_texture, NULL, &rect); // Рисуем фигуру
            }
        }

        // Рисуем подсветку клеток
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0); // Устанавливаем цвет подсветки
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale); // Масштабируем рендер
        for (POS_T i = 0; i < 8; ++i) {
            for (POS_T j = 0; j < 8; ++j) {
                if (!is_highlighted_[i][j]) continue; // Пропускаем не подсвеченные клетки
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale), int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell); // Рисуем рамку вокруг клетки
            }
        }

        // Рисуем активную клетку
        if (active_x != -1) {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0); // Устанавливаем красный цвет для активной клетки
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale), int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell); // Рисуем рамку вокруг активной клетки
        }

        SDL_RenderSetScale(ren, 1, 1); // Возвращаем масштаб рендера к норме

        // Рисуем кнопки "Назад" и "Перезапуск"
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 };
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // Рисуем результат игры
        if (game_results != -1) {
            string result_path = draw_path; // Выбираем изображение результата
            if (game_results == 1) result_path = white_path;
            else if (game_results == 2) result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr) { // Проверка загрузки текстуры
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect); // Рисуем результат
            SDL_DestroyTexture(result_texture); // Освобождаем текстуру
        }

        SDL_RenderPresent(ren); // Отображаем все нарисованное
        SDL_Delay(10); // Задержка для корректного отображения
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent); // Обработка событий окна
    }

    // Метод для записи ошибок в лог-файл
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app); // Открываем лог-файл
        fout << "Error: " << text << ". " << SDL_GetError() << endl; // Записываем ошибку
        fout.close();
    }

public:
    int W = 0; // Ширина окна
    int H = 0; // Высота окна

    // История состояний доски
    vector<vector<vector<POS_T>>> history_mtx;

private:
    SDL_Window* win = nullptr; // Указатель на окно
    SDL_Renderer* ren = nullptr; // Указатель на рендер

    // Текстуры для доски, фигур и кнопок
    SDL_Texture* board = nullptr;
    SDL_Texture* w_piece = nullptr;
    SDL_Texture* b_piece = nullptr;
    SDL_Texture* w_queen = nullptr;
    SDL_Texture* b_queen = nullptr;
    SDL_Texture* back = nullptr;
    SDL_Texture* replay = nullptr;

    // Пути к текстурам
    const string textures_path = project_path + "Textures/";
    const string board_path = textures_path + "board.png";
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png";
    const string queen_white_path = textures_path + "queen_white.png";
    const string queen_black_path = textures_path + "queen_black.png";
    const string white_path = textures_path + "white_wins.png";
    const string black_path = textures_path + "black_wins.png";
    const string draw_path = textures_path + "draw.png";
    const string back_path = textures_path + "back.png";
    const string replay_path = textures_path + "replay.png";

    // Координаты активной клетки
    int active_x = -1, active_y = -1;

    // Результат игры (-1 - игра продолжается, 1 - победа белых, 2 - победа черных, 0 - ничья)
    int game_results = -1;

    // Матрица подсветки клеток
    vector<vector<int>> is_highlighted_ = vector<vector<int>>(8, vector<int>(8, 0));

    // Текущая матрица доски
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));

    // Серии ударов для каждого хода
    vector<int> history_beat_series;
};