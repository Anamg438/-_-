#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>
#include <sys/sysinfo.h>

// 1.Температура проца
double getCpuTemperature() {
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (!tempFile.is_open()) {
        return -1.0; // Датчик недоступен
    }
    double tempMilli = 0;
    tempFile >> tempMilli;
    return tempMilli / 1000.0;
}

// 2. ОЗУ
void printSystemInfo() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        double totalRamMB = (info.totalram * info.mem_unit) / (1024.0 * 1024.0);
        double freeRamMB = (info.freeram * info.mem_unit) / (1024.0 * 1024.0);
        std::cout << "[Система] Доступно ядер CPU: " << std::thread::hardware_concurrency() << "\n";
        std::cout << "[Система] Оперативная память: " << freeRamMB << " МБ свободно из " << totalRamMB << " МБ\n";
        std::cout << "[Система] Время работы (Uptime): " << info.uptime << " сек.\n";
    }
}

// 3. Бенчмарк
void runCpuBenchmark() {
    std::cout << "\n[Тест CPU] Запуск математического теста (5 млн итераций)...\n";
    auto start = std::chrono::high_resolution_clock::now();

    double result = 0.0;
    for (long i = 1; i <= 5000000; ++i) {
        result += std::sin(i) * std::cos(i) / std::sqrt(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "[Тест CPU] Готово за " << elapsed.count() << " мс (контрольное значение: " << result << ")\n";
}

// 4.GPIO через sysfs Linux
void testGpioBlink(int gpioPin, int blinks) {
    std::string gpioPath = "/sys/class/gpio/gpio" + std::to_string(gpioPin);

    // Экспортируем пин
    std::ofstream exportFile("/sys/class/gpio/export");
    if (exportFile.is_open()) {
        exportFile << gpioPin;
        exportFile.close();
    }

    //100 мс на создание файлов пина
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Настраиваем пин как выход (out)
    std::ofstream dirFile(gpioPath + "/direction");
    if (!dirFile.is_open()) {
        std::cout << "[GPIO] Не удалось открыть GPIO " << gpioPin
            << ". Проверьте права (нужен sudo/root) или номер пина.\n";
        return;
    }
    dirFile << "out";
    dirFile.close();

    std::cout << "\n[GPIO] Мигаем пином GPIO " << gpioPin << " (" << blinks << " раз)...\n";
    std::ofstream valFile(gpioPath + "/value");
    for (int i = 0; i < blinks; ++i) {
        valFile << "1";
        valFile.flush();
        std::cout << "  -> ВКЛ (1)\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        valFile << "0";
        valFile.flush();
        std::cout << "  -> ВЫКЛ (0)\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    valFile.close();

    // Освобождаем пин
    std::ofstream unexportFile("/sys/class/gpio/unexport");
    if (unexportFile.is_open()) {
        unexportFile << gpioPin;
        unexportFile.close();
    }
}

int main() {
    std::cout << "=== Тест возможностей Luckfox Lyra Ultra W ===\n\n";

    // Вывод системной информации и температуры до нагрузки
    printSystemInfo();
    double tempBefore = getCpuTemperature();
    if (tempBefore > 0) {
        std::cout << "[Датчик] Температура чипа RK3506B: " << tempBefore << " °C\n";
    }

    // Нагрузочный тест процессора
    runCpuBenchmark();

    // Температура после нагрузки
    double tempAfter = getCpuTemperature();
    if (tempAfter > 0) {
        std::cout << "[Датчик] Температура после теста: " << tempAfter << " °C\n";
    }

    // Тест GPIO (схема Luckfox Wiki)
    testGpioBlink(54, 3);

    std::cout << "\n=== Тестирование успешно завершено! ===\n";
    return 0;
}