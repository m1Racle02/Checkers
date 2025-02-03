#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "../Models/Project_path.h"

class Config
{
public:
    // ����������� ������ Config. ��� �������� ������� ������������� �������� ����� reload()
    // ��� �������� �������� �� ����� settings.json.
    Config()
    {
        reload();
    }

    // ����� reload() ������������� ��������� �� ����� settings.json.
    // �� ��������� ����, ��������� ��� ���������� � ������� JSON � ��������� ��� � ��������� ���������� config.
    // ���� ���� ��� ������� ��� ������, ���� ����� ������������ ������������ ������ � ���������.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json"); // ��������� ���� settings.json
        fin >> config; // ��������� ���������� ����� � ������ JSON
        fin.close(); // ��������� ���� ����� ������
    }

    // �������� () (�������������� ��������) ��������� ���������� � ���������� ��� � ��������� ���������.
    // ��������� ��� ���������: setting_dir (�����������/������ ��������) � setting_name (��� ���������� ���������).
    // ���������� �������� ��������� �� ���������������� ������� JSON-�����.
    // ��������, config("Bot", "IsWhiteBot") ������ �������� ��������� IsWhiteBot �� ������� Bot.
    auto operator()(const std::string& setting_dir, const std::string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config; // ��������� ���������� ��� �������� �������� � ������� JSON
};