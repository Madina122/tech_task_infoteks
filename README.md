# EEPROM 25LC040A SPI Driver

Драйвер для микросхемы EEPROM 25LC040A с интерфейсом SPI через bit-banging.

## Документация 

- **[EEPROM 25LC040A](https://www.infinite-electronic.ru/datasheet/7f-25LC040AT-I-ST.pdf)** 
- **[NOR Flash (W25Q128)](https://static.chipdip.ru/lib/093/DOC001093213.pdf)** 

## Структура проекта
```bash
├── eeprom/                      
│   ├── include/     
│   │   ├── eeprom_25lc040a.h    
│   │   └── spi_bitbang_driver.h            
│   └── src    
│       └── eeprom_25lc040a.cpp        
├── test/                       
│   └── test_eeprom.cpp       
├── README.md                   
└── CMakeLists.txt                      
```

## Особенности реализации

1. **Полный функционал:**
   - Чтение/запись отдельных битов
   - Чтение/запись байтов
   - Чтение/запись массивов байт

2. **Автоматическая обработка:**
   - Разбиение записи по границам страниц (16 байт)
   - RMW (Read-Modify-Write) цикл для операций с битами
   - Ожидание завершения операций записи

3. **Обработка ошибок:**
   - Проверка границ адресов (0-511)
   - Проверка позиций битов (0-7)
   - Исключения для невалидных параметров

## Сборка и запуск

### Требования
- CMake 3.10 или выше
- Компилятор C++17 (g++, clang++, MSVC)

### Инструкция по сборке

```bash
# 1. Клонирование (если из репозитория)
git clone <repository-url>
cd EEPROM_25LC040A_Driver

# 2. Создание папки для сборки
mkdir build
cd build

# 3. Конфигурация проекта
cmake ..

# 4. Сборка
cmake --build .

# 5. Запуск тестового примера
./test_eeprom.exe  # на Windows
./test_eeprom      # на Linux/macOS
```

## Отличия для NOR Flash (W25Q128)

| Характеристики | 	EEPROM 25LC040A | NOR Flash W25Q128 |
|------|----------------|----|
| Размер | 512 байт | 16 MБ (128 Mбит)|
| Блоки памяти | нет | Sector (4KB), Block (64KB), Chip Erase |
| Запись | Byte/Page | Только после стирания |
| Стирание | Автоматическое | Sector/Block/Chip |  
| Адрес | 16 бит | 24 бита |
| Базовые инстуркции | 6  | 40+  | 

### 1. Необходимо добавить новые константы 

```C++
static constexpr size_t MEMORY_SIZE = 16 * 1024 * 1024;  // 16MB
static constexpr size_t SECTOR_SIZE = 4 * 1024;          // 4KB sectors
static constexpr size_t PAGE_SIZE = 256;                 // 256B pages
static constexpr uint32_t ERASE_TIMEOUT_MS = 500;        // Sector erase ~400ms
```

### 2. Добавить как минимум еще 9 команд

```C++
enum class NorCommand : uint8_t {
    READ              = 0x03,  // Чтение данных
    PAGE_PROGRAM      = 0x02,  // Запись 256B после стирания
    SECTOR_ERASE      = 0x20,  // Стирание сектора 4KB
    BLOCK32K_ERASE    = 0x52,  // Стирание блока 32KB
    BLOCK64K_ERASE    = 0xD8,  // Стирание блока 64KB
    CHIP_ERASE        = 0xC7,  // Стирание всего чипа
    WRITE_ENABLE      = 0x06,  // Разрешить запись
    READ_STATUS1      = 0x05,  // Чтение Status Register 1
    RESET             = 0xFF   // Software Reset
};
```

### 3. Реализовать новые методы

```C++
/**
 * @brief Erase sector (4KB) before writing
 */
bool eraseSector(uint32_t sectorAddress);

/**
 * @brief Erase entire chip (~40 seconds!)
 */
bool eraseChip();

/**
 * @brief Check if memory is erased (0xFF)
 */
bool isErased(uint32_t address, size_t size);
```

### 4. Расширить статус регистров
### 5. Переделать логику записи (через стирания)

## Документация

Все классы, методы и константы документированы в формате Doxygen. Для генерации HTML документации:

1. Установите Doxygen (если не установлен)    
Через официальный сайт    
`sudo apt install doxygen`  # Ubuntu/Debian    
`brew install doxygen`      # macOS

2. Генерация документации    
`doxygen -g Doxyfile`

3. Отредактируйте Doxyfile, указав INPUT = eeprom/include eeprom/src    

4. Откройте `docs/html/index.html` или `html/index.html`
