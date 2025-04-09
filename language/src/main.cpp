#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

// Подключите свои заголовочные файлы для парсера, транслятора и сериализатора.
// Например:
#include <json/serializer.hpp>
#include <parser/parser.hpp>
#include <translator/translator.hpp>

namespace fs = std::filesystem;

// Функция для обрезки пробелов по краям строки
constexpr std::string Trim(std::string_view const input)
{
    auto view = input | std::views::drop_while(isspace) | std::views::reverse |
                std::views::drop_while(isspace) | std::views::reverse;
    return {view.begin(), view.end()};
}

int main(int argc, char *argv[])
{
    std::string usage = "Usage: " + std::string(argv[0]) +
                        " [-f <input_file>|-] [-o <output_file>|-] -t <json|cypher>\n"
                        "       use '-' for stdin/stdout mode.";

    if (argc < 3)
    {
        std::cerr << usage << std::endl;
        return 1;
    }

    bool inputProvided = false;
    bool outputProvided = false;
    fs::path inputPath;
    fs::path outputPath;
    std::string saveType;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc)
        {
            inputProvided = true;
            std::string inArg = argv[++i];
            if (inArg == "-")
            {
                inputPath = "-";
            }
            else
            {
                inputPath = fs::path(inArg);
            }
        }
        else if (arg == "-o" && i + 1 < argc)
        {
            outputProvided = true;
            std::string outArg = argv[++i];
            if (outArg == "-")
            {
                outputPath = "-";
            }
            else
            {
                outputPath = fs::path(outArg);
            }
        }
        else if (arg == "-t" && i + 1 < argc)
        {
            saveType = argv[++i];
        }
        else
        {
            std::cerr << "Неизвестный аргумент: " << arg << std::endl;
            std::cerr << usage << std::endl;
            return 1;
        }
    }

    if (saveType != "json" && saveType != "cypher")
    {
        std::cerr << "Ошибка: тип сохранения должен быть 'json' или 'cypher'." << std::endl;
        return 1;
    }

    std::string fileContent;
    if (!inputProvided || inputPath == "-")
    {
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        fileContent = buffer.str();
    }
    else
    {
        if (!fs::exists(inputPath))
        {
            std::cerr << "Входной файл не существует: " << inputPath << std::endl;
            return 1;
        }
        std::ifstream inFile(inputPath);
        if (!inFile)
        {
            std::cerr << "Не удалось открыть входной файл: " << inputPath << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        fileContent = buffer.str();
    }

    std::string trimmedContent = Trim(fileContent);

    auto data = lang::grammar::Parse(trimmedContent);
    if (!data.has_value())
    {
        std::cerr << "Ошибка парсинга." << std::endl;
        return 1;
    }

    std::string output;
    if (saveType == "json")
    {
        output = lang::ast::json::Serialize(data.value());
    }
    else if (saveType == "cypher")
    {
        output = lang::ast::cypher::Translate(data.value());
    }

    if (!outputProvided || outputPath == "-")
    {
        std::cout << output;
    }
    else
    {
        std::ofstream outFile(outputPath);
        if (!outFile)
        {
            std::cerr << "Output file is not open: " << outputPath << std::endl;
            return 1;
        }
        outFile << output;
    }

    return 0;
}