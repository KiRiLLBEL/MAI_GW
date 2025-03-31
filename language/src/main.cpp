#include <fstream>
#include <iostream>
#include <json/serializer.hpp>
#include <parser/parser.hpp>
#include <translator/translator.hpp>

namespace fs = std::filesystem;

constexpr std::string Trim(std::string_view const input)
{
    auto view = input | std::views::drop_while(isspace) | std::views::reverse |
                std::views::drop_while(isspace) | std::views::reverse;
    return {view.begin(), view.end()};
}

static constexpr auto maxArgsCount = 7;

int main(int argc, char *argv[])
{
    if (argc < maxArgsCount)
    {
        std::cerr << "Usage: " << argv[0]
                  << " -f <input_file> -o <output_file> -t <json|cypher>" << std::endl;
        return 1;
    }

    fs::path inputPath;
    fs::path outputPath;
    std::string saveType;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc)
        {
            inputPath = fs::path(argv[++i]);
        }
        else if (arg == "-o" && i + 1 < argc)
        {
            outputPath = fs::path(argv[++i]);
        }
        else if (arg == "-t" && i + 1 < argc)
        {
            saveType = argv[++i];
        }
        else
        {
            std::cerr << "Неизвестный аргумент: " << arg << std::endl;
            return 1;
        }
    }

    if (saveType != "json" && saveType != "cypher")
    {
        std::cerr << "Ошибка: тип сохранения должен быть 'json' или 'cypher'." << std::endl;
        return 1;
    }

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
    std::string fileContent = buffer.str();

    std::string trimmedContent = Trim(fileContent);

    const auto data = lang::grammar::Parse(trimmedContent);

    std::string output{};
    std::ofstream outputFile(outputPath);
    if (saveType == "json")
    {
        output = lang::ast::json::Serialize(data.value());
    }
    else if (saveType == "cypher")
    {
        output = lang::ast::cypher::Translate(data.value());
    }

    outputFile << output;

    std::cout << "success" << std::endl;
    return 0;
}