#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "../Models/Project_path.h"

class Config
{
public:
    // Конструктор класса Config. При создании объекта автоматически вызывает метод reload()
    // для загрузки настроек из файла settings.json.
    Config()
    {
        reload();
    }

    // Метод reload() перезагружает настройки из файла settings.json.
    // Он открывает файл, считывает его содержимое в формате JSON и сохраняет его в приватную переменную config.
    // Если файл был изменен или удален, этот метод обеспечивает актуальность данных в программе.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json"); // Открываем файл settings.json
        fin >> config; // Считываем содержимое файла в объект JSON
        fin.close(); // Закрываем файл после чтения
    }

    // Оператор () (функциональный оператор) позволяет обращаться к настройкам как к элементам структуры.
    // Принимает два параметра: setting_dir (направление/раздел настроек) и setting_name (имя конкретной настройки).
    // Возвращает значение настройки из соответствующего раздела JSON-файла.
    // Например, config("Bot", "IsWhiteBot") вернет значение настройки IsWhiteBot из раздела Bot.
    auto operator()(const std::string& setting_dir, const std::string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config; // Приватная переменная для хранения настроек в формате JSON
};